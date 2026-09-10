// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Derivation of the LocalSettings value name used to persist one window's
// placement.
//
// See docs/design-notes/Window-PlacementPersistence.md section 3.1. The raw
// Window.PersistPlacementId is never used directly as a value name:
//
//     valueName = "wp1_" + slug(rawId) + "_" + Base32(SHA-256(UTF-16LE(rawId)))
//
// The slug is diagnostic only. The hash supplies uniqueness and makes arbitrary
// app strings syntactically safe as value names.

#include <windows.h>
#include <stdint.h>
#include <string>

namespace WindowPlacementPersistence {

// The container inside LocalSettings that holds every persisted placement.
constexpr WCHAR c_settingsContainerName[] = L"Microsoft.UI.Xaml.WindowPlacement";

// Value-name prefix. The "1" tracks the value-name scheme, not the blob version.
constexpr WCHAR c_valueNamePrefix[] = L"wp1_";

// The slug keeps at most this many accepted characters.
constexpr size_t c_maxSlugLength = 16;

// Used when the raw id contains no ASCII alphanumeric characters at all.
constexpr WCHAR c_defaultSlug[] = L"id";

// SHA-256 produces 32 bytes, which Base32-encodes to 52 characters with no padding.
constexpr size_t c_hashByteCount = 32;
constexpr size_t c_base32HashLength = 52;

// Upper bound on the raw id length we will hash, in UTF-16 code units. No real
// id comes close; this exists so the byte count always fits a DWORD.
constexpr size_t c_maxPlacementIdLength = 32768;

// Scans `rawId` left to right, keeping only ASCII [A-Za-z0-9], and stops after
// c_maxSlugLength accepted characters. Produces "id" when nothing is accepted.
std::wstring MakeSlug(_In_reads_(length) const WCHAR* rawId, size_t length);

// Uppercase RFC 4648 Base32 with no padding. Base32 is used rather than base64
// because LocalSettings value names are case-insensitive, so two distinct
// base64 encodings could collide as names.
std::wstring Base32Encode(_In_reads_(size) const uint8_t* bytes, size_t size);

// Computes SHA-256 over the raw UTF-16 code units of `rawId`, interpreted as
// little-endian bytes with no trailing NUL.
//
// This uses CryptHashCertificate2 with BCRYPT_SHA256_ALGORITHM. Despite the
// name, that API hashes an arbitrary byte block through CNG using Crypt32.dll,
// which microsoft.ui.xaml.dll already imports. That avoids both a new bcrypt.lib
// link dependency and a private SHA-256 implementation.
//
// The hash provides a stable, negligibly collision-prone value-name component.
// It is not used for authentication, integrity, or confidentiality. A collision
// would select the same placement slot within the app's own settings store; it
// does not cross an application security boundary.
bool ComputeSha256(
    _In_reads_(length) const WCHAR* rawId,
    size_t length,
    _Out_writes_(c_hashByteCount) uint8_t* hash);

// Builds the full value name. Returns false when `rawId` is empty or when
// hashing fails, in which case the caller treats the window as not participating.
bool TryGetPlacementValueName(
    _In_reads_(length) const WCHAR* rawId,
    size_t length,
    _Out_ std::wstring& valueName);

} // namespace WindowPlacementPersistence
