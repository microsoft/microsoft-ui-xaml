// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacementValueName.h"
#include "WindowPlacementFormat.h"

#include <windows.h>
#include <wincrypt.h>
#include <bcrypt.h> // BCRYPT_SHA256_ALGORITHM string constant only; no bcrypt.lib link needed

namespace WindowPlacement
{
    namespace
    {
        bool IsSlugChar(wchar_t ch)
        {
            return (ch >= L'A' && ch <= L'Z') ||
                   (ch >= L'a' && ch <= L'z') ||
                   (ch >= L'0' && ch <= L'9');
        }

        // Uppercase RFC 4648 Base32 alphabet.
        constexpr wchar_t c_base32Alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

        std::wstring Base32Encode(const uint8_t* bytes, size_t length)
        {
            std::wstring out;
            out.reserve((length * 8 + 4) / 5);

            uint32_t buffer = 0;
            int bitsLeft = 0;
            for (size_t i = 0; i < length; ++i)
            {
                buffer = (buffer << 8) | bytes[i];
                bitsLeft += 8;
                while (bitsLeft >= 5)
                {
                    bitsLeft -= 5;
                    out.push_back(c_base32Alphabet[(buffer >> bitsLeft) & 0x1F]);
                }
            }
            if (bitsLeft > 0)
            {
                out.push_back(c_base32Alphabet[(buffer << (5 - bitsLeft)) & 0x1F]);
            }
            return out;
        }

        // SHA-256 of an arbitrary byte block via CNG through Crypt32 (already imported). This
        // avoids a new bcrypt.lib link dependency and a private SHA-256 implementation. See design
        // section 3.1. Returns false only if the platform call fails.
        bool ComputeSha256(const uint8_t* bytes, size_t length, uint8_t (&hash)[32])
        {
            DWORD hashLength = sizeof(hash);
            if (!CryptHashCertificate2(
                    BCRYPT_SHA256_ALGORITHM,
                    0,          // dwFlags
                    nullptr,    // pvReserved
                    bytes,
                    static_cast<DWORD>(length),
                    hash,
                    &hashLength))
            {
                return false;
            }
            return hashLength == sizeof(hash);
        }
    }

    std::wstring MakePlacementSlug(const std::wstring& rawId)
    {
        std::wstring slug;
        for (wchar_t ch : rawId)
        {
            if (IsSlugChar(ch))
            {
                slug.push_back(ch);
                if (slug.size() >= c_maxSlugLength)
                {
                    break;
                }
            }
        }
        if (slug.empty())
        {
            slug = c_emptySlug;
        }
        return slug;
    }

    std::wstring MakePlacementHash(const std::wstring& rawId)
    {
        // Hash the raw id's UTF-16LE code units, with no trailing NUL. On Windows wchar_t is
        // already UTF-16LE, so the string's bytes are the code units directly.
        uint8_t hash[32] = {};
        const auto* bytes = reinterpret_cast<const uint8_t*>(rawId.data());
        const size_t byteLength = rawId.size() * sizeof(wchar_t);
        if (!ComputeSha256(bytes, byteLength, hash))
        {
            return std::wstring();
        }
        return Base32Encode(hash, sizeof(hash));
    }

    std::wstring DerivePlacementValueName(const std::wstring& rawId)
    {
        const std::wstring hash = MakePlacementHash(rawId);
        if (hash.empty())
        {
            return std::wstring(); // platform hash failure -> no usable storage slot
        }

        std::wstring valueName;
        valueName.reserve(wcslen(c_valueNamePrefix) + c_maxSlugLength + 1 + hash.size());
        valueName += c_valueNamePrefix;
        valueName += MakePlacementSlug(rawId);
        valueName += L'_';
        valueName += hash;
        return valueName;
    }
}
