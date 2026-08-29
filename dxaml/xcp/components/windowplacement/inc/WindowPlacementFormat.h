// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

// Wire format for a persisted WinUI window placement.
//
// WinUI owns this serialized format. See docs/design-notes/Window-PlacementPersistence.md,
// section 3.4 "Persistence data format". The blob stored in ApplicationData.LocalSettings is
// this binary envelope, Base64-encoded.
//
// Binary layout (all integers little-endian):
//
//   Header (12 bytes)
//     char     magic[4]     ASCII 'W','P','L','1'
//     uint16   major        must-understand version; a reader rejects an unknown major
//     uint16   minor        additive version; a reader tolerates an unknown minor
//     uint32   totalLength  total blob length in bytes, including this header
//
//   Field (repeated until totalLength is reached)
//     uint16   tag          one of WindowPlacementTag
//     uint16   length       length of the value that follows, in bytes
//     byte     value[length]
//
// Rect coordinates are signed int32. Other scalar values are unsigned uint32. Device names are
// raw UTF-16LE code units with no trailing NUL. A virtual-desktop id uses the Windows GUID field
// layout (LE uint32 Data1, LE uint16 Data2, LE uint16 Data3, then the eight Data4 bytes).

namespace WindowPlacement
{
    // Header magic: ASCII "WPL1".
    inline constexpr uint8_t c_headerMagic[4] = { 'W', 'P', 'L', '1' };
    inline constexpr size_t c_headerMagicLength = 4;

    // Header is magic(4) + major(2) + minor(2) + totalLength(4).
    inline constexpr size_t c_headerLength = 12;

    // A field prefix is tag(2) + length(2).
    inline constexpr size_t c_fieldHeaderLength = 4;

    // v1 version numbers.
    inline constexpr uint16_t c_majorVersionV1 = 1;
    inline constexpr uint16_t c_minorVersionV1 = 0;

    // Tag ids. The writer emits present tags once, in ascending order, producing a canonical
    // blob. The reader does not require that order and skips unknown tags by their encoded length.
    enum class WindowPlacementTag : uint16_t
    {
        NormalRect       = 0x0001, // four int32: left, top, right, bottom (required)
        WorkArea         = 0x0002, // four int32
        ArrangeRect      = 0x0003, // four int32
        Dpi              = 0x0010, // uint32
        ShowCmd          = 0x0011, // uint32 Win32 SW_* value
        Flags            = 0x0012, // uint32 WinUI-owned bitfield (WindowPlacementFlags)
        DeviceName       = 0x0020, // UTF-16LE code units, no trailing NUL
        VirtualDesktopId = 0x0021, // Windows GUID field layout
    };

    // Fixed value lengths, in bytes.
    inline constexpr uint16_t c_rectValueLength = 16;   // four int32
    inline constexpr uint16_t c_uint32ValueLength = 4;
    inline constexpr uint16_t c_guidValueLength = 16;

    // A device name is UTF-16LE with no NUL. Win32 GDI device names are at most 32 code units
    // (CCHDEVICENAME), including a terminating NUL, so 31 code units of content -> 62 bytes.
    inline constexpr uint16_t c_maxDeviceNameValueLength = 62;

    // Durable flag bits (WinUI wire meanings). These are the contract WinUI owns; their initial
    // values match the corresponding PlacementEx flags to keep translation trivial, but the
    // storage contract belongs to WinUI. See design section 3.4.2.
    enum WindowPlacementFlags : uint32_t
    {
        WindowPlacementFlags_None                    = 0x0000,
        WindowPlacementFlags_RestoreToMaximized      = 0x0001,
        WindowPlacementFlags_Arranged                = 0x0002,
        WindowPlacementFlags_AllowPartiallyOffScreen = 0x0004,
        WindowPlacementFlags_AllowSizing             = 0x0008,
        WindowPlacementFlags_RestoreToArranged       = 0x0020,

        // All bits WinUI understands in v1. Bits outside this mask are ignored on read and never
        // written.
        WindowPlacementFlags_KnownMask =
            WindowPlacementFlags_RestoreToMaximized |
            WindowPlacementFlags_Arranged |
            WindowPlacementFlags_AllowPartiallyOffScreen |
            WindowPlacementFlags_AllowSizing |
            WindowPlacementFlags_RestoreToArranged,
    };

    // Coordinates outside this magnitude are treated as corruption. A saved rectangle beyond this
    // range cannot describe a real monitor arrangement.
    inline constexpr int32_t c_maxCoordinateMagnitude = 1000000;

    // Upper bound on a decoded blob, enforced before allocation. A valid v1 blob is far smaller;
    // this only guards against a hostile or corrupt length field.
    inline constexpr size_t c_maxBlobLength = 4096;
}
