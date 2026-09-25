// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "WindowPlacementStorageFormat.h"

#include <windows.h>
#include <bcrypt.h>
#include <limits>
#include <utility>

namespace DirectUI::WindowPlacementPersistence
{
    namespace
    {
        constexpr char16_t Base64Alphabet[] =
            u"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        // Uppercase RFC 4648 base32. Base64 alphabets are unusable in names that may compare
        // case-insensitively in the settings backend.
        constexpr char16_t Base32Alphabet[] = u"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        constexpr size_t DigestBytes = 32;
        constexpr size_t MaximumSlugCharacters = 16;

        int Base64Value(char16_t character) noexcept
        {
            if (character >= u'A' && character <= u'Z') return character - u'A';
            if (character >= u'a' && character <= u'z') return character - u'a' + 26;
            if (character >= u'0' && character <= u'9') return character - u'0' + 52;
            if (character == u'+') return 62;
            if (character == u'/') return 63;
            return -1;
        }

        bool IsSlugCharacter(char16_t character) noexcept
        {
            return (character >= u'A' && character <= u'Z') ||
                (character >= u'a' && character <= u'z') ||
                (character >= u'0' && character <= u'9');
        }

        // Hash the exact UTF-16 code units: no terminator, case folding, or normalization.
        bool TryComputeDigest(const char16_t* text, size_t length, uint8_t (&digest)[DigestBytes])
        {
            constexpr size_t maxUnits = (std::numeric_limits<ULONG>::max)() / sizeof(char16_t);
            if (length > maxUnits) return false;
            // BCryptHash takes a non-const input buffer but does not modify it.
            const auto status = BCryptHash(
                BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char16_t*>(text)),
                static_cast<ULONG>(length * sizeof(char16_t)),
                digest, static_cast<ULONG>(DigestBytes));
            return BCRYPT_SUCCESS(status);
        }

        void AppendDigest(std::u16string& name, const uint8_t (&digest)[DigestBytes])
        {
            uint32_t accumulator = 0;
            int bits = 0;
            for (auto value : digest)
            {
                accumulator = (accumulator << 8) | value;
                bits += 8;
                while (bits >= 5)
                {
                    bits -= 5;
                    name.push_back(Base32Alphabet[(accumulator >> bits) & 0x1f]);
                }
            }
            // 256 bits is not a multiple of five; emit the remainder without padding.
            if (bits > 0) name.push_back(Base32Alphabet[(accumulator << (5 - bits)) & 0x1f]);
        }
    }

    bool EncodeRecordText(const Record& record, std::u16string& text)
    {
        std::vector<uint8_t> bytes;
        if (!EncodeRecord(record, bytes)) return false;

        std::u16string result;
        result.reserve(((bytes.size() + 2) / 3) * 4);
        for (size_t i = 0; i < bytes.size(); i += 3)
        {
            const auto remaining = bytes.size() - i;
            const uint32_t group = (uint32_t{bytes[i]} << 16) |
                (remaining > 1 ? uint32_t{bytes[i + 1]} << 8 : 0) |
                (remaining > 2 ? uint32_t{bytes[i + 2]} : 0);
            result.push_back(Base64Alphabet[(group >> 18) & 0x3f]);
            result.push_back(Base64Alphabet[(group >> 12) & 0x3f]);
            result.push_back(remaining > 1 ? Base64Alphabet[(group >> 6) & 0x3f] : u'=');
            result.push_back(remaining > 2 ? Base64Alphabet[group & 0x3f] : u'=');
        }
        text = std::move(result);
        return true;
    }

    DecodeResult DecodeRecordText(const char16_t* text, size_t length, Record& record)
    {
        // Bound the encoded value before allocating a decode buffer.
        if (!text || length == 0 || length > MaximumEncodedCharacters || length % 4 != 0)
        {
            return DecodeResult::InvalidData;
        }

        size_t padding = 0;
        while (padding < 2 && text[length - 1 - padding] == u'=') ++padding;

        std::vector<uint8_t> bytes;
        bytes.reserve(length / 4 * 3);
        uint32_t group = 0;
        const auto dataCharacters = length - padding;
        for (size_t i = 0; i < dataCharacters; ++i)
        {
            const auto value = Base64Value(text[i]);
            if (value < 0) return DecodeResult::InvalidData; // Includes misplaced padding.
            group = (group << 6) | static_cast<uint32_t>(value);
            if (i % 4 == 3)
            {
                bytes.push_back(static_cast<uint8_t>(group >> 16));
                bytes.push_back(static_cast<uint8_t>(group >> 8));
                bytes.push_back(static_cast<uint8_t>(group));
                group = 0;
            }
        }

        // Require canonical unused bits so one record has exactly one encoding.
        if (padding == 1)
        {
            if ((group & 0x3) != 0) return DecodeResult::InvalidData;
            bytes.push_back(static_cast<uint8_t>(group >> 10));
            bytes.push_back(static_cast<uint8_t>(group >> 2));
        }
        else if (padding == 2)
        {
            if ((group & 0xf) != 0) return DecodeResult::InvalidData;
            bytes.push_back(static_cast<uint8_t>(group >> 4));
        }

        return DecodeRecord(bytes.data(), bytes.size(), record);
    }

    bool TryMakeApplicationContainerName(const char16_t* applicationId, size_t length, std::u16string& name)
    {
        if (!applicationId || length == 0) return false;
        uint8_t digest[DigestBytes];
        if (!TryComputeDigest(applicationId, length, digest)) return false;

        std::u16string result = u"app1_";
        AppendDigest(result, digest);
        name = std::move(result);
        return true;
    }

    bool TryMakePlacementValueName(const char16_t* placementId, size_t length, std::u16string& name)
    {
        if (!placementId || length == 0) return false;
        uint8_t digest[DigestBytes];
        if (!TryComputeDigest(placementId, length, digest)) return false;

        // The slug is a diagnostic aid only; the digest supplies uniqueness.
        std::u16string slug;
        for (size_t i = 0; i < length && slug.size() < MaximumSlugCharacters; ++i)
        {
            if (IsSlugCharacter(placementId[i])) slug.push_back(placementId[i]);
        }
        if (slug.empty()) slug = u"id";

        std::u16string result = u"wp1_";
        result += slug;
        result.push_back(u'_');
        AppendDigest(result, digest);
        name = std::move(result);
        return true;
    }

    bool TryMakePlacementGroupPropertyName(
        const char16_t* applicationId,
        size_t applicationIdLength,
        const char16_t* placementId,
        size_t placementIdLength,
        std::u16string& name)
    {
        if ((!applicationId && applicationIdLength != 0) ||
            !placementId ||
            placementIdLength == 0)
        {
            return false;
        }

        try
        {
            // Length-prefix both components so embedded NULs cannot create an
            // alternate representation of the exact, case-sensitive key.
            std::u16string input;
            if (applicationIdLength > (std::numeric_limits<uint32_t>::max)() ||
                placementIdLength > (std::numeric_limits<uint32_t>::max)())
            {
                return false;
            }
            input.reserve(applicationIdLength + placementIdLength + 4);
            const auto appendLength = [&input](size_t length)
            {
                const auto value = static_cast<uint32_t>(length);
                input.push_back(static_cast<char16_t>(value & 0xffff));
                input.push_back(static_cast<char16_t>(value >> 16));
            };
            appendLength(applicationIdLength);
            input.append(applicationId ? applicationId : u"", applicationIdLength);
            appendLength(placementIdLength);
            input.append(placementId, placementIdLength);

            uint8_t digest[DigestBytes];
            if (!TryComputeDigest(input.data(), input.size(), digest)) return false;

            std::u16string result = u"Microsoft.UI.Xaml.PlacementGroup.v1.";
            AppendDigest(result, digest);
            name = std::move(result);
            return true;
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
    }
}
