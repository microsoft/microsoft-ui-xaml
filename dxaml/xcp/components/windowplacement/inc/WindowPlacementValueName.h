// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <string>

// Derive the deterministic LocalSettings value name for a raw PersistPlacementId. See
// docs/design-notes/Window-PlacementPersistence.md section 3.1 "Loading the persistence data".
//
//   valueName = "wp1_" + slug(rawId) + "_" + Base32(SHA-256(UTF-16LE(rawId)))
//
// The raw id is never used directly as a value name. The readable slug is diagnostic only; the
// hash supplies uniqueness and makes an arbitrary app string safe as a case-insensitive
// LocalSettings value name.

namespace WindowPlacement
{
    // Value-name prefix. Ties the value name to this format generation.
    inline constexpr wchar_t c_valueNamePrefix[] = L"wp1_";

    // Slug: scan the raw id left to right, keep only ASCII [A-Za-z0-9], and stop after 16 accepted
    // characters. If no character is accepted, the slug is "id".
    inline constexpr size_t c_maxSlugLength = 16;
    inline constexpr wchar_t c_emptySlug[] = L"id";

    // Uppercase RFC 4648 Base32 (no padding). A SHA-256 digest is 32 bytes -> 52 Base32 chars.
    inline constexpr size_t c_hashBase32Length = 52;

    // Compute the readable slug for a raw id. Public for testing and diagnostics.
    std::wstring MakePlacementSlug(const std::wstring& rawId);

    // Compute the uppercase, unpadded RFC 4648 Base32 of the SHA-256 of the raw id's UTF-16LE code
    // units (no trailing NUL). Returns an empty string only if the platform hash call fails.
    std::wstring MakePlacementHash(const std::wstring& rawId);

    // Derive the full LocalSettings value name for a raw PersistPlacementId. Returns an empty
    // string only if the platform hash call fails, in which case the caller treats the placement
    // as having no usable storage slot.
    std::wstring DerivePlacementValueName(const std::wstring& rawId);
}
