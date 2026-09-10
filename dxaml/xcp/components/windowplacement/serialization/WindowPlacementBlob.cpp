// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "WindowPlacementBlob.h"

#include <wincrypt.h>
#include <string.h>

namespace WindowPlacementPersistence {

namespace {

// ---------------------------------------------------------------------------
// Little-endian primitives
// ---------------------------------------------------------------------------

void AppendU16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void AppendU32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void AppendI32(std::vector<uint8_t>& out, int32_t value)
{
    AppendU32(out, static_cast<uint32_t>(value));
}

uint16_t ReadU16(const uint8_t* bytes)
{
    return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
}

uint32_t ReadU32(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

int32_t ReadI32(const uint8_t* bytes)
{
    return static_cast<int32_t>(ReadU32(bytes));
}

// ---------------------------------------------------------------------------
// Rect helpers
// ---------------------------------------------------------------------------

void AppendRect(std::vector<uint8_t>& out, const RECT& rect)
{
    AppendI32(out, rect.left);
    AppendI32(out, rect.top);
    AppendI32(out, rect.right);
    AppendI32(out, rect.bottom);
}

RECT ReadRect(const uint8_t* bytes)
{
    RECT rect{};
    rect.left   = ReadI32(bytes);
    rect.top    = ReadI32(bytes + 4);
    rect.right  = ReadI32(bytes + 8);
    rect.bottom = ReadI32(bytes + 12);
    return rect;
}

bool IsCoordinateInRange(int32_t value)
{
    return value >= -c_maxCoordinate && value <= c_maxCoordinate;
}

// Compared against directly rather than using GUID_NULL, which would add a
// uuid.lib link dependency to this otherwise self-contained library.
bool IsNullGuid(const GUID& id)
{
    if (id.Data1 != 0 || id.Data2 != 0 || id.Data3 != 0)
    {
        return false;
    }

    for (size_t i = 0; i < ARRAYSIZE(id.Data4); i++)
    {
        if (id.Data4[i] != 0)
        {
            return false;
        }
    }

    return true;
}

// Every present rectangle must have positive width and height, and every
// coordinate must be within +/-1,000,000 pixels.
bool IsRectValid(const RECT& rect)
{
    if (!IsCoordinateInRange(rect.left) ||
        !IsCoordinateInRange(rect.top) ||
        !IsCoordinateInRange(rect.right) ||
        !IsCoordinateInRange(rect.bottom))
    {
        return false;
    }

    return rect.right > rect.left && rect.bottom > rect.top;
}

// A checked test that scaling this rect from `dpi` up to the largest DPI we
// accept cannot overflow 32-bit arithmetic. Coordinates are already bounded, so
// this is defense in depth against a blob that pairs a huge rect with a tiny DPI.
bool WouldDpiScalingOverflow(const RECT& rect, uint32_t dpi)
{
    if (dpi < c_minDpi)
    {
        return true;
    }

    const int64_t width  = static_cast<int64_t>(rect.right) - rect.left;
    const int64_t height = static_cast<int64_t>(rect.bottom) - rect.top;

    const int64_t scaledWidth  = width * c_maxDpi / dpi;
    const int64_t scaledHeight = height * c_maxDpi / dpi;

    constexpr int64_t limit = static_cast<int64_t>(INT32_MAX);
    return scaledWidth > limit || scaledHeight > limit;
}

} // namespace

// ---------------------------------------------------------------------------
// Show command canonicalization
// ---------------------------------------------------------------------------

uint32_t CanonicalizeShowCommand(uint32_t showCmd)
{
    switch (showCmd)
    {
    // Normal aliases. SW_SHOWNA displays the window in its current size and
    // position, which for a freshly created window is the normal state.
    case SW_SHOWNORMAL:      // same value as SW_NORMAL
    case SW_SHOWNOACTIVATE:
    case SW_SHOW:
    case SW_SHOWNA:
    case SW_RESTORE:
    case SW_SHOWDEFAULT:
        return SW_NORMAL;

    // Maximized aliases. SW_MAXIMIZE has the same value as SW_SHOWMAXIMIZED.
    case SW_SHOWMAXIMIZED:
        return SW_SHOWMAXIMIZED;

    // Minimized aliases.
    case SW_SHOWMINIMIZED:
    case SW_MINIMIZE:
    case SW_SHOWMINNOACTIVE:
    case SW_FORCEMINIMIZE:
        return SW_SHOWMINIMIZED;

    // SW_HIDE, any other non-displaying command, and unknown values are treated
    // as an absent tag, which defaults to SW_NORMAL.
    default:
        return SW_NORMAL;
    }
}

// ---------------------------------------------------------------------------
// Writer
// ---------------------------------------------------------------------------

bool SerializePlacement(const PlacementBlobData& data, std::vector<uint8_t>& blob)
{
    // Never persist something our own reader would reject.
    if (!IsRectValid(data.normalRect) || !IsRectValid(data.workArea))
    {
        return false;
    }

    if (data.dpi < c_minDpi || data.dpi > c_maxDpi)
    {
        return false;
    }

    if (WouldDpiScalingOverflow(data.normalRect, data.dpi) ||
        WouldDpiScalingOverflow(data.workArea, data.dpi))
    {
        return false;
    }

    const uint32_t flags = static_cast<uint32_t>(data.flags) & c_knownDurableFlags;

    // Arranged and RestoreToArranged both depend on a valid arrange rect.
    const bool needsArrangeRect =
        (flags & static_cast<uint32_t>(DurableFlags::Arranged)) != 0 ||
        (flags & static_cast<uint32_t>(DurableFlags::RestoreToArranged)) != 0;

    if (needsArrangeRect && (!data.hasArrangeRect || !IsRectValid(data.arrangeRect)))
    {
        return false;
    }

    const bool writeArrangeRect = needsArrangeRect;

    // Device names are written without a trailing NUL and must fit the wire cap.
    size_t deviceNameChars = 0;
    while (deviceNameChars < CCHDEVICENAME && data.deviceName[deviceNameChars] != L'\0')
    {
        deviceNameChars++;
    }

    if (deviceNameChars * sizeof(WCHAR) > c_maxDeviceNameBytes)
    {
        return false;
    }

    const bool writeDeviceName = deviceNameChars > 0;

    const bool writeVirtualDesktopId =
        data.hasVirtualDesktopId && !IsNullGuid(data.virtualDesktopId);

    std::vector<uint8_t> body;

    // Emit each applicable tag once, in ascending tag order, so the encoding is
    // canonical and can be compared against a checked-in golden blob.
    auto appendTagHeader = [&body](Tag tag, uint16_t length)
    {
        AppendU16(body, static_cast<uint16_t>(tag));
        AppendU16(body, length);
    };

    appendTagHeader(Tag::NormalRect, 16);
    AppendRect(body, data.normalRect);

    appendTagHeader(Tag::WorkArea, 16);
    AppendRect(body, data.workArea);

    if (writeArrangeRect)
    {
        appendTagHeader(Tag::ArrangeRect, 16);
        AppendRect(body, data.arrangeRect);
    }

    appendTagHeader(Tag::Dpi, 4);
    AppendU32(body, data.dpi);

    // The show command is always canonical on the wire.
    appendTagHeader(Tag::ShowCmd, 4);
    AppendU32(body, CanonicalizeShowCommand(data.showCmd));

    appendTagHeader(Tag::Flags, 4);
    AppendU32(body, flags);

    if (writeDeviceName)
    {
        appendTagHeader(Tag::DeviceName, static_cast<uint16_t>(deviceNameChars * sizeof(WCHAR)));
        for (size_t i = 0; i < deviceNameChars; i++)
        {
            AppendU16(body, static_cast<uint16_t>(data.deviceName[i]));
        }
    }

    if (writeVirtualDesktopId)
    {
        appendTagHeader(Tag::VirtualDesktopId, 16);
        AppendU32(body, data.virtualDesktopId.Data1);
        AppendU16(body, data.virtualDesktopId.Data2);
        AppendU16(body, data.virtualDesktopId.Data3);
        for (size_t i = 0; i < 8; i++)
        {
            body.push_back(data.virtualDesktopId.Data4[i]);
        }
    }

    const size_t totalLength = c_headerSize + body.size();
    if (totalLength > c_maxBlobSize)
    {
        return false;
    }

    std::vector<uint8_t> result;
    result.reserve(totalLength);
    result.insert(result.end(), c_magic, c_magic + sizeof(c_magic));
    AppendU16(result, c_versionMajor);
    AppendU16(result, c_versionMinor);
    AppendU32(result, static_cast<uint32_t>(totalLength));
    result.insert(result.end(), body.begin(), body.end());

    blob = std::move(result);
    return true;
}

// ---------------------------------------------------------------------------
// Reader
// ---------------------------------------------------------------------------

bool DeserializePlacement(const uint8_t* bytes, size_t size, PlacementBlobData& data)
{
    data = PlacementBlobData{};

    if (bytes == nullptr || size < c_headerSize || size > c_maxBlobSize)
    {
        return false;
    }

    if (memcmp(bytes, c_magic, sizeof(c_magic)) != 0)
    {
        return false;
    }

    const uint16_t major = ReadU16(bytes + 4);
    const uint32_t totalLength = ReadU32(bytes + 8);

    // A higher minor version with major version 1 is accepted; its unknown
    // additive tags are skipped below. A different major version means a
    // must-understand change, so the blob is rejected.
    if (major != c_versionMajor)
    {
        return false;
    }

    // The total length must exactly match the decoded byte count.
    if (totalLength != size)
    {
        return false;
    }

    PlacementBlobData parsed;

    bool sawNormalRect = false;
    bool sawWorkArea   = false;
    bool sawDpi        = false;

    // Duplicate tags are accepted with last-one-wins semantics, although the
    // WinUI writer never emits duplicates.
    size_t offset = c_headerSize;
    while (offset < size)
    {
        // Every field is bounds-checked before it is read.
        if (size - offset < 4)
        {
            return false;
        }

        const uint16_t tag = ReadU16(bytes + offset);
        const uint16_t length = ReadU16(bytes + offset + 2);
        offset += 4;

        if (size - offset < length)
        {
            return false;
        }

        const uint8_t* value = bytes + offset;

        switch (static_cast<Tag>(tag))
        {
        case Tag::NormalRect:
            // A known fixed-width tag with the wrong length rejects the blob.
            if (length != 16) { return false; }
            parsed.normalRect = ReadRect(value);
            sawNormalRect = true;
            break;

        case Tag::WorkArea:
            if (length != 16) { return false; }
            parsed.workArea = ReadRect(value);
            sawWorkArea = true;
            break;

        case Tag::ArrangeRect:
            if (length != 16) { return false; }
            parsed.arrangeRect = ReadRect(value);
            parsed.hasArrangeRect = true;
            break;

        case Tag::Dpi:
            if (length != 4) { return false; }
            parsed.dpi = ReadU32(value);
            sawDpi = true;
            break;

        case Tag::ShowCmd:
            if (length != 4) { return false; }
            // Non-displaying and unknown commands canonicalize to SW_NORMAL,
            // which is also the absent-tag default.
            parsed.showCmd = CanonicalizeShowCommand(ReadU32(value));
            break;

        case Tag::Flags:
            if (length != 4) { return false; }
            // Unknown bits are ignored and never passed through to PlacementEx.
            parsed.flags = static_cast<DurableFlags>(ReadU32(value) & c_knownDurableFlags);
            break;

        case Tag::DeviceName:
        {
            // Length must be even and no greater than 62 bytes.
            if ((length % sizeof(WCHAR)) != 0 || length > c_maxDeviceNameBytes)
            {
                return false;
            }

            const size_t charCount = length / sizeof(WCHAR);

            // Zero length is treated as absent.
            if (charCount == 0)
            {
                parsed.deviceName[0] = L'\0';
                break;
            }

            for (size_t i = 0; i < charCount; i++)
            {
                const WCHAR ch = static_cast<WCHAR>(ReadU16(value + (i * sizeof(WCHAR))));

                // Embedded NUL characters reject the blob.
                if (ch == L'\0')
                {
                    return false;
                }

                parsed.deviceName[i] = ch;
            }
            parsed.deviceName[charCount] = L'\0';
            break;
        }

        case Tag::VirtualDesktopId:
        {
            // Virtual-desktop ids must be exactly 16 bytes.
            if (length != 16) { return false; }

            GUID id{};
            id.Data1 = ReadU32(value);
            id.Data2 = ReadU16(value + 4);
            id.Data3 = ReadU16(value + 6);
            memcpy(id.Data4, value + 8, 8);

            // GUID_NULL is treated as an absent tag.
            if (IsNullGuid(id))
            {
                parsed.virtualDesktopId = GUID{};
                parsed.hasVirtualDesktopId = false;
            }
            else
            {
                parsed.virtualDesktopId = id;
                parsed.hasVirtualDesktopId = true;
            }
            break;
        }

        default:
            // Unknown tags are skipped using their encoded length.
            break;
        }

        offset += length;
    }

    // The final field must end exactly at the total-length boundary.
    if (offset != size)
    {
        return false;
    }

    // Missing any required tag means no saved placement.
    if (!sawNormalRect || !sawWorkArea || !sawDpi)
    {
        return false;
    }

    if (!IsRectValid(parsed.normalRect) || !IsRectValid(parsed.workArea))
    {
        return false;
    }

    if (parsed.hasArrangeRect && !IsRectValid(parsed.arrangeRect))
    {
        return false;
    }

    if (parsed.dpi < c_minDpi || parsed.dpi > c_maxDpi)
    {
        return false;
    }

    if (WouldDpiScalingOverflow(parsed.normalRect, parsed.dpi) ||
        WouldDpiScalingOverflow(parsed.workArea, parsed.dpi))
    {
        return false;
    }

    // Arranged and RestoreToArranged require a valid arrange rect. Rather than
    // rejecting the whole blob, drop those flags: the rest of the placement is
    // still usable and this degrades to a normal restore.
    if (!parsed.hasArrangeRect)
    {
        const uint32_t withoutArranged =
            static_cast<uint32_t>(parsed.flags) &
            ~(static_cast<uint32_t>(DurableFlags::Arranged) |
              static_cast<uint32_t>(DurableFlags::RestoreToArranged));
        parsed.flags = static_cast<DurableFlags>(withoutArranged);
    }

    data = parsed;
    return true;
}

// ---------------------------------------------------------------------------
// Base64
// ---------------------------------------------------------------------------

bool Base64Encode(const std::vector<uint8_t>& blob, std::wstring& text)
{
    text.clear();

    if (blob.empty() || blob.size() > c_maxBlobSize)
    {
        return false;
    }

    DWORD charCount = 0;
    if (!CryptBinaryToStringW(
            blob.data(),
            static_cast<DWORD>(blob.size()),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            nullptr,
            &charCount) ||
        charCount == 0)
    {
        return false;
    }

    // charCount includes the terminating NUL.
    std::wstring buffer(charCount, L'\0');
    if (!CryptBinaryToStringW(
            blob.data(),
            static_cast<DWORD>(blob.size()),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            &buffer[0],
            &charCount))
    {
        return false;
    }

    // charCount is now the length excluding the terminating NUL.
    buffer.resize(charCount);
    text = std::move(buffer);
    return true;
}

bool Base64Decode(const WCHAR* text, size_t length, std::vector<uint8_t>& blob)
{
    blob.clear();

    // Base64 of the largest legal blob is under 5.5 KB. Anything longer cannot
    // decode to something we would accept, so reject it before doing any work.
    if (text == nullptr || length == 0 || length > c_maxEncodedTextLength)
    {
        return false;
    }

    DWORD byteCount = 0;
    if (!CryptStringToBinaryW(
            text,
            static_cast<DWORD>(length),
            CRYPT_STRING_BASE64,
            nullptr,
            &byteCount,
            nullptr,
            nullptr))
    {
        return false;
    }

    // Cap the decoded size before allocating.
    if (byteCount == 0 || byteCount > c_maxBlobSize)
    {
        return false;
    }

    std::vector<uint8_t> buffer(byteCount);
    if (!CryptStringToBinaryW(
            text,
            static_cast<DWORD>(length),
            CRYPT_STRING_BASE64,
            buffer.data(),
            &byteCount,
            nullptr,
            nullptr))
    {
        return false;
    }

    buffer.resize(byteCount);
    blob = std::move(buffer);
    return true;
}

bool TryEncodePlacement(const PlacementBlobData& data, std::wstring& text)
{
    text.clear();

    std::vector<uint8_t> blob;
    if (!SerializePlacement(data, blob))
    {
        return false;
    }

    return Base64Encode(blob, text);
}

bool TryDecodePlacement(const WCHAR* text, size_t length, PlacementBlobData& data)
{
    data = PlacementBlobData{};

    std::vector<uint8_t> blob;
    if (!Base64Decode(text, length, blob))
    {
        return false;
    }

    return DeserializePlacement(blob.data(), blob.size(), data);
}

} // namespace WindowPlacementPersistence
