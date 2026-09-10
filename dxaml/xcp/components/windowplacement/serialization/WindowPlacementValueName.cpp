// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WindowPlacementValueName.h"

#include <wincrypt.h>
#include <bcrypt.h>

namespace WindowPlacementPersistence {

namespace {

// RFC 4648 Base32 alphabet, uppercase.
constexpr WCHAR c_base32Alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

bool IsAsciiAlphanumeric(WCHAR ch)
{
    return (ch >= L'0' && ch <= L'9') ||
           (ch >= L'A' && ch <= L'Z') ||
           (ch >= L'a' && ch <= L'z');
}

} // namespace

std::wstring MakeSlug(const WCHAR* rawId, size_t length)
{
    std::wstring slug;
    slug.reserve(c_maxSlugLength);

    if (rawId != nullptr)
    {
        // Scan left to right, keeping only ASCII alphanumerics, and stop once
        // the slug is full. Everything else is dropped, not replaced, so the
        // slug never introduces separator characters of its own.
        for (size_t i = 0; i < length && slug.size() < c_maxSlugLength; i++)
        {
            if (IsAsciiAlphanumeric(rawId[i]))
            {
                slug.push_back(rawId[i]);
            }
        }
    }

    if (slug.empty())
    {
        slug = c_defaultSlug;
    }

    return slug;
}

std::wstring Base32Encode(const uint8_t* bytes, size_t size)
{
    std::wstring result;

    if (bytes == nullptr || size == 0)
    {
        return result;
    }

    // Each 5 bits of input produce one character. A trailing partial group is
    // zero-filled and emitted; no padding characters are appended.
    result.reserve(((size * 8) + 4) / 5);

    uint32_t accumulator = 0;
    int bitsHeld = 0;

    for (size_t i = 0; i < size; i++)
    {
        accumulator = (accumulator << 8) | bytes[i];
        bitsHeld += 8;

        while (bitsHeld >= 5)
        {
            bitsHeld -= 5;
            const uint32_t index = (accumulator >> bitsHeld) & 0x1F;
            result.push_back(c_base32Alphabet[index]);
        }
    }

    if (bitsHeld > 0)
    {
        const uint32_t index = (accumulator << (5 - bitsHeld)) & 0x1F;
        result.push_back(c_base32Alphabet[index]);
    }

    return result;
}

bool ComputeSha256(const WCHAR* rawId, size_t length, uint8_t* hash)
{
    if (rawId == nullptr || length == 0 || length > c_maxPlacementIdLength || hash == nullptr)
    {
        return false;
    }

    const size_t byteLength = length * sizeof(WCHAR);
    // WCHAR is UTF-16LE on Windows, so the in-memory bytes already are the
    // little-endian encoding the format specifies. No trailing NUL is hashed.
    DWORD hashSize = static_cast<DWORD>(c_hashByteCount);
    if (!CryptHashCertificate2(
            BCRYPT_SHA256_ALGORITHM,
            0,          // dwFlags
            nullptr,    // pvReserved
            reinterpret_cast<const BYTE*>(rawId),
            static_cast<DWORD>(byteLength),
            hash,
            &hashSize))
    {
        return false;
    }

    return hashSize == c_hashByteCount;
}

bool TryGetPlacementValueName(const WCHAR* rawId, size_t length, std::wstring& valueName)
{
    valueName.clear();

    // An empty id means the window does not participate.
    if (rawId == nullptr || length == 0)
    {
        return false;
    }

    uint8_t hash[c_hashByteCount] = {};
    if (!ComputeSha256(rawId, length, hash))
    {
        return false;
    }

    const std::wstring encodedHash = Base32Encode(hash, c_hashByteCount);
    if (encodedHash.size() != c_base32HashLength)
    {
        return false;
    }

    std::wstring result;
    result.reserve(ARRAYSIZE(c_valueNamePrefix) + c_maxSlugLength + 1 + c_base32HashLength);
    result.append(c_valueNamePrefix);
    result.append(MakeSlug(rawId, length));
    result.push_back(L'_');
    result.append(encodedHash);

    valueName = std::move(result);
    return true;
}

} // namespace WindowPlacementPersistence
