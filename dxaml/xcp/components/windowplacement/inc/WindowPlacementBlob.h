// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Serialization of persisted window placement.
//
// WinUI stores one base64 string in LocalSettings containing a versioned binary
// TLV envelope. Apps never inspect or modify the value; WinUI owns the format.
//
// See docs/design-notes/Window-PlacementPersistence.md section 3.4 for the
// authoritative description of the wire format. This header intentionally has no
// dependency on PlacementEx so that the format can be unit tested on its own.

#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace WindowPlacementPersistence {

// Durable flag bits, as written to the wire. These are WinUI wire meanings. The
// initial values match the equivalent PlacementEx values to keep translation
// simple, but the storage contract belongs to WinUI and may diverge later.
//
// Bit 0x0010 is intentionally unused: the analogous PlacementEx bit is the
// call-time-only KeepHidden instruction. PlacementEx FullScreen (0x0040),
// VirtualDesktopId (0x0080), and NoActivate (0x0100) are never persisted as
// flags. Virtual-desktop identity uses its own tag.
enum class DurableFlags : uint32_t
{
    None                    = 0x0000,
    RestoreToMaximized      = 0x0001,
    Arranged                = 0x0002,
    AllowPartiallyOffScreen = 0x0004,
    AllowSizing             = 0x0008,
    RestoreToArranged       = 0x0020,
};

// The full set of bits a v1 reader understands. Unknown bits are dropped and are
// never passed through to PlacementEx.
constexpr uint32_t c_knownDurableFlags =
    static_cast<uint32_t>(DurableFlags::RestoreToMaximized) |
    static_cast<uint32_t>(DurableFlags::Arranged) |
    static_cast<uint32_t>(DurableFlags::AllowPartiallyOffScreen) |
    static_cast<uint32_t>(DurableFlags::AllowSizing) |
    static_cast<uint32_t>(DurableFlags::RestoreToArranged);

inline DurableFlags operator|(DurableFlags a, DurableFlags b)
{
    return static_cast<DurableFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline DurableFlags& operator|=(DurableFlags& a, DurableFlags b)
{
    a = a | b;
    return a;
}

inline bool HasFlag(DurableFlags value, DurableFlags flag)
{
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

// v1 tag values.
enum class Tag : uint16_t
{
    NormalRect       = 0x0001,
    WorkArea         = 0x0002,
    ArrangeRect      = 0x0003,
    Dpi              = 0x0010,
    ShowCmd          = 0x0011,
    Flags            = 0x0012,
    DeviceName       = 0x0020,
    VirtualDesktopId = 0x0021,
};

// Format constants. Exposed so tests can assert against them directly.
constexpr uint8_t  c_magic[4]        = { 'W', 'P', 'L', '1' };
constexpr uint16_t c_versionMajor    = 1;
constexpr uint16_t c_versionMinor    = 0;
constexpr size_t   c_headerSize      = 12;    // magic(4) + major(2) + minor(2) + totalLength(4)
constexpr size_t   c_maxBlobSize     = 4096;  // 4 KiB cap applied before allocation
constexpr size_t   c_maxDeviceNameBytes = 62; // 31 UTF-16 code units, no NUL

// Base64 of the largest legal blob is under 5.5 KB. Longer text cannot decode to
// anything we would accept, so it is rejected up front.
constexpr size_t   c_maxEncodedTextLength = 8192;

// Rectangle coordinates outside this range reject the blob.
constexpr int32_t  c_maxCoordinate   = 1000000;

// DPI bounds. 96 is 100% scaling; the upper bound is far above any real display
// and exists so that later DPI scaling arithmetic cannot overflow.
constexpr uint32_t c_minDpi          = 96;
constexpr uint32_t c_maxDpi          = 9600;

// The decoded contents of a placement blob. This mirrors only the fields WinUI
// persists, which is a subset of PlacementEx state.
struct PlacementBlobData
{
    // Required fields.
    RECT     normalRect = {};
    RECT     workArea   = {};
    uint32_t dpi        = 0;

    // Optional fields, with their defaults applied by the reader.
    uint32_t     showCmd = SW_NORMAL;
    DurableFlags flags   = DurableFlags::None;

    RECT arrangeRect    = {};
    bool hasArrangeRect = false;

    // NUL terminated. Empty when absent.
    WCHAR deviceName[CCHDEVICENAME] = {};

    GUID virtualDesktopId    = {};
    bool hasVirtualDesktopId = false;
};

// Writes the canonical v1 encoding of `data`. Tags are emitted at most once, in
// ascending tag order, so the output is stable enough for a golden test.
//
// Returns false without touching `blob` if `data` is not something a reader would
// accept, so the writer can never persist a blob its own reader would reject.
bool SerializePlacement(_In_ const PlacementBlobData& data, _Out_ std::vector<uint8_t>& blob);

// Reads a v1 (or compatible higher-minor) blob. Returns false for any blob that
// fails the fail-safe rules in design section 3.4.3, in which case the caller
// behaves as though no placement was saved.
bool DeserializePlacement(_In_reads_(size) const uint8_t* bytes, size_t size, _Out_ PlacementBlobData& data);

// Base64 helpers over the same byte blob. These are the outer layer stored in
// LocalSettings.
bool Base64Encode(_In_ const std::vector<uint8_t>& blob, _Out_ std::wstring& text);
bool Base64Decode(_In_reads_(length) const WCHAR* text, size_t length, _Out_ std::vector<uint8_t>& blob);

// Convenience wrappers combining serialization and base64.
bool TryEncodePlacement(_In_ const PlacementBlobData& data, _Out_ std::wstring& text);
bool TryDecodePlacement(_In_reads_(length) const WCHAR* text, size_t length, _Out_ PlacementBlobData& data);

// Canonicalizes a Win32 SW_* show command to one of SW_NORMAL, SW_SHOWMAXIMIZED,
// or SW_SHOWMINIMIZED. Non-displaying and unknown commands canonicalize to
// SW_NORMAL, which is also what an absent tag produces.
uint32_t CanonicalizeShowCommand(uint32_t showCmd);

} // namespace WindowPlacementPersistence
