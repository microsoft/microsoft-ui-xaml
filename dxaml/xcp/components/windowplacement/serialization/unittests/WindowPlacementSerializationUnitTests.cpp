// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacementSerializationUnitTests.h"

#include "WindowPlacementBlob.h"
#include "WindowPlacementValueName.h"

using namespace WindowPlacementPersistence;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace WindowPlacement {

namespace {

// Golden encodings. These are computed independently of the implementation and
// must not be regenerated from it. Changing either constant means the persisted
// format changed, which requires a version bump and a compatibility review.
//
// Minimal: normalRect {100,200,900,800}, workArea {0,0,1920,1080}, dpi 96,
//          showCmd SW_NORMAL, flags none.
const uint8_t c_goldenMinimalBlob[] = {
    0x57, 0x50, 0x4C, 0x31,                         // "WPL1"
    0x01, 0x00,                                     // major 1
    0x00, 0x00,                                     // minor 0
    0x4C, 0x00, 0x00, 0x00,                         // total length 76
    0x01, 0x00, 0x10, 0x00,                         // TAG_NORMAL_RECT, len 16
    0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00,
    0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x10, 0x00,                         // TAG_WORK_AREA, len 16
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x07, 0x00, 0x00, 0x38, 0x04, 0x00, 0x00,
    0x10, 0x00, 0x04, 0x00, 0x60, 0x00, 0x00, 0x00, // TAG_DPI = 96
    0x11, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, // TAG_SHOW_CMD = SW_NORMAL
    0x12, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, // TAG_FLAGS = 0
};

// Full: adds arrangeRect {110,210,890,790}, dpi 192, showCmd SW_SHOWMAXIMIZED,
//       flags RestoreToMaximized|Arranged|AllowSizing, device name "\\.\DISPLAY1",
//       virtual desktop id {12345678-9ABC-DEF0-0123-456789ABCDEF}.
const uint8_t c_goldenFullBlob[] = {
    0x57, 0x50, 0x4C, 0x31,
    0x01, 0x00,
    0x00, 0x00,
    0x90, 0x00, 0x00, 0x00,                         // total length 144
    0x01, 0x00, 0x10, 0x00,
    0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00,
    0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x07, 0x00, 0x00, 0x38, 0x04, 0x00, 0x00,
    0x03, 0x00, 0x10, 0x00,                         // TAG_ARRANGE_RECT, len 16
    0x6E, 0x00, 0x00, 0x00, 0xD2, 0x00, 0x00, 0x00,
    0x7A, 0x03, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00,
    0x10, 0x00, 0x04, 0x00, 0xC0, 0x00, 0x00, 0x00, // TAG_DPI = 192
    0x11, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, // TAG_SHOW_CMD = SW_SHOWMAXIMIZED
    0x12, 0x00, 0x04, 0x00, 0x0B, 0x00, 0x00, 0x00, // TAG_FLAGS = 0x000B
    0x20, 0x00, 0x18, 0x00,                         // TAG_DEVICE_NAME, len 24
    0x5C, 0x00, 0x5C, 0x00, 0x2E, 0x00, 0x5C, 0x00,
    0x44, 0x00, 0x49, 0x00, 0x53, 0x00, 0x50, 0x00,
    0x4C, 0x00, 0x41, 0x00, 0x59, 0x00, 0x31, 0x00,
    0x21, 0x00, 0x10, 0x00,                         // TAG_VIRTUAL_DESKTOP_ID, len 16
    0x78, 0x56, 0x34, 0x12, 0xBC, 0x9A, 0xF0, 0xDE,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
};

const GUID c_goldenVirtualDesktopId =
    { 0x12345678, 0x9ABC, 0xDEF0, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } };

RECT MakeRect(LONG left, LONG top, LONG right, LONG bottom)
{
    RECT rect{};
    rect.left = left;
    rect.top = top;
    rect.right = right;
    rect.bottom = bottom;
    return rect;
}

PlacementBlobData MakeMinimalData()
{
    PlacementBlobData data;
    data.normalRect = MakeRect(100, 200, 900, 800);
    data.workArea = MakeRect(0, 0, 1920, 1080);
    data.dpi = 96;
    data.showCmd = SW_NORMAL;
    data.flags = DurableFlags::None;
    return data;
}

PlacementBlobData MakeFullData()
{
    PlacementBlobData data;
    data.normalRect = MakeRect(100, 200, 900, 800);
    data.workArea = MakeRect(0, 0, 1920, 1080);
    data.arrangeRect = MakeRect(110, 210, 890, 790);
    data.hasArrangeRect = true;
    data.dpi = 192;
    data.showCmd = SW_SHOWMAXIMIZED;
    data.flags = DurableFlags::RestoreToMaximized | DurableFlags::Arranged | DurableFlags::AllowSizing;
    wcscpy_s(data.deviceName, L"\\\\.\\DISPLAY1");
    data.virtualDesktopId = c_goldenVirtualDesktopId;
    data.hasVirtualDesktopId = true;
    return data;
}

void VerifyBytesEqual(const std::vector<uint8_t>& actual, const uint8_t* expected, size_t expectedSize)
{
    VERIFY_ARE_EQUAL(expectedSize, actual.size());

    const size_t count = (expectedSize < actual.size()) ? expectedSize : actual.size();
    for (size_t i = 0; i < count; i++)
    {
        if (actual[i] != expected[i])
        {
            WEX::Common::String message;
            message.Format(
                L"Byte mismatch at offset %u: expected 0x%02X, got 0x%02X",
                static_cast<unsigned>(i),
                static_cast<unsigned>(expected[i]),
                static_cast<unsigned>(actual[i]));
            WEX::Logging::Log::Comment(message);
        }
        VERIFY_ARE_EQUAL(expected[i], actual[i]);
    }
}

void VerifyRectsEqual(const RECT& expected, const RECT& actual)
{
    VERIFY_ARE_EQUAL(expected.left, actual.left);
    VERIFY_ARE_EQUAL(expected.top, actual.top);
    VERIFY_ARE_EQUAL(expected.right, actual.right);
    VERIFY_ARE_EQUAL(expected.bottom, actual.bottom);
}

// Helpers for building hand-crafted blobs in the negative tests.

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

void AppendTag(std::vector<uint8_t>& out, Tag tag, uint16_t length)
{
    AppendU16(out, static_cast<uint16_t>(tag));
    AppendU16(out, length);
}

void AppendRect(std::vector<uint8_t>& out, const RECT& rect)
{
    AppendU32(out, static_cast<uint32_t>(rect.left));
    AppendU32(out, static_cast<uint32_t>(rect.top));
    AppendU32(out, static_cast<uint32_t>(rect.right));
    AppendU32(out, static_cast<uint32_t>(rect.bottom));
}

// Wraps a body in a valid header, fixing up the total length.
std::vector<uint8_t> WrapBody(const std::vector<uint8_t>& body, uint16_t major = 1, uint16_t minor = 0)
{
    std::vector<uint8_t> blob;
    blob.insert(blob.end(), c_magic, c_magic + sizeof(c_magic));
    AppendU16(blob, major);
    AppendU16(blob, minor);
    AppendU32(blob, static_cast<uint32_t>(c_headerSize + body.size()));
    blob.insert(blob.end(), body.begin(), body.end());
    return blob;
}

// The required tags, as raw body bytes.
std::vector<uint8_t> MakeRequiredBody(uint32_t dpi = 96)
{
    std::vector<uint8_t> body;
    AppendTag(body, Tag::NormalRect, 16);
    AppendRect(body, MakeRect(100, 200, 900, 800));
    AppendTag(body, Tag::WorkArea, 16);
    AppendRect(body, MakeRect(0, 0, 1920, 1080));
    AppendTag(body, Tag::Dpi, 4);
    AppendU32(body, dpi);
    return body;
}

bool Deserialize(const std::vector<uint8_t>& blob, PlacementBlobData& data)
{
    return DeserializePlacement(blob.data(), blob.size(), data);
}

} // namespace

// ===========================================================================
// Blob tests
// ===========================================================================

void WindowPlacementBlobUnitTests::GoldenMinimalBlob()
{
    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(MakeMinimalData(), blob));
    VerifyBytesEqual(blob, c_goldenMinimalBlob, ARRAYSIZE(c_goldenMinimalBlob));
}

void WindowPlacementBlobUnitTests::GoldenFullBlob()
{
    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(MakeFullData(), blob));
    VerifyBytesEqual(blob, c_goldenFullBlob, ARRAYSIZE(c_goldenFullBlob));
}

void WindowPlacementBlobUnitTests::RoundTripsMinimalPlacement()
{
    const PlacementBlobData original = MakeMinimalData();

    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(original, blob));

    PlacementBlobData decoded;
    VERIFY_IS_TRUE(Deserialize(blob, decoded));

    VerifyRectsEqual(original.normalRect, decoded.normalRect);
    VerifyRectsEqual(original.workArea, decoded.workArea);
    VERIFY_ARE_EQUAL(original.dpi, decoded.dpi);
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), decoded.showCmd);
    VERIFY_ARE_EQUAL(0u, static_cast<uint32_t>(decoded.flags));
    VERIFY_IS_FALSE(decoded.hasArrangeRect);
    VERIFY_ARE_EQUAL(L'\0', decoded.deviceName[0]);
    VERIFY_IS_FALSE(decoded.hasVirtualDesktopId);
}

void WindowPlacementBlobUnitTests::RoundTripsFullPlacement()
{
    const PlacementBlobData original = MakeFullData();

    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(original, blob));

    PlacementBlobData decoded;
    VERIFY_IS_TRUE(Deserialize(blob, decoded));

    VerifyRectsEqual(original.normalRect, decoded.normalRect);
    VerifyRectsEqual(original.workArea, decoded.workArea);
    VERIFY_IS_TRUE(decoded.hasArrangeRect);
    VerifyRectsEqual(original.arrangeRect, decoded.arrangeRect);
    VERIFY_ARE_EQUAL(original.dpi, decoded.dpi);
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_SHOWMAXIMIZED), decoded.showCmd);
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(original.flags), static_cast<uint32_t>(decoded.flags));
    VERIFY_ARE_EQUAL(0, wcscmp(original.deviceName, decoded.deviceName));
    VERIFY_IS_TRUE(decoded.hasVirtualDesktopId);
    VERIFY_IS_TRUE(decoded.virtualDesktopId == c_goldenVirtualDesktopId);
}

void WindowPlacementBlobUnitTests::RoundTripsThroughBase64()
{
    const PlacementBlobData original = MakeFullData();

    std::wstring text;
    VERIFY_IS_TRUE(TryEncodePlacement(original, text));
    VERIFY_IS_GREATER_THAN(text.size(), static_cast<size_t>(0));

    // The encoded text must be a single line so it round trips cleanly through
    // settings storage.
    VERIFY_ARE_EQUAL(std::wstring::npos, text.find(L'\r'));
    VERIFY_ARE_EQUAL(std::wstring::npos, text.find(L'\n'));

    PlacementBlobData decoded;
    VERIFY_IS_TRUE(TryDecodePlacement(text.c_str(), text.size(), decoded));

    VerifyRectsEqual(original.normalRect, decoded.normalRect);
    VERIFY_ARE_EQUAL(original.dpi, decoded.dpi);
    VERIFY_IS_TRUE(decoded.virtualDesktopId == c_goldenVirtualDesktopId);
}

void WindowPlacementBlobUnitTests::RejectsBadHeader()
{
    PlacementBlobData decoded;

    // Null and empty.
    VERIFY_IS_FALSE(DeserializePlacement(nullptr, 0, decoded));
    VERIFY_IS_FALSE(DeserializePlacement(c_goldenMinimalBlob, 0, decoded));

    // Shorter than the header.
    for (size_t size = 1; size < c_headerSize; size++)
    {
        VERIFY_IS_FALSE(DeserializePlacement(c_goldenMinimalBlob, size, decoded));
    }

    // Wrong magic.
    std::vector<uint8_t> wrongMagic(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    wrongMagic[3] = '2';
    VERIFY_IS_FALSE(Deserialize(wrongMagic, decoded));

    // Wrong major version. A must-understand change means old readers reject.
    std::vector<uint8_t> wrongMajor(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    wrongMajor[4] = 2;
    VERIFY_IS_FALSE(Deserialize(wrongMajor, decoded));

    // Major version 0.
    std::vector<uint8_t> zeroMajor(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    zeroMajor[4] = 0;
    VERIFY_IS_FALSE(Deserialize(zeroMajor, decoded));
}

void WindowPlacementBlobUnitTests::RejectsWrongTotalLength()
{
    PlacementBlobData decoded;

    // Too small.
    std::vector<uint8_t> tooSmall(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    tooSmall[8] = 0x4B;
    VERIFY_IS_FALSE(Deserialize(tooSmall, decoded));

    // Too large.
    std::vector<uint8_t> tooLarge(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    tooLarge[8] = 0x4D;
    VERIFY_IS_FALSE(Deserialize(tooLarge, decoded));

    // Absurd.
    std::vector<uint8_t> absurd(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    absurd[8] = 0xFF; absurd[9] = 0xFF; absurd[10] = 0xFF; absurd[11] = 0xFF;
    VERIFY_IS_FALSE(Deserialize(absurd, decoded));
}

void WindowPlacementBlobUnitTests::RejectsTruncatedBlob()
{
    PlacementBlobData decoded;

    // Truncating at any offset must fail cleanly. The stored total length no
    // longer matches, and every field read is bounds-checked.
    for (size_t size = 1; size < ARRAYSIZE(c_goldenFullBlob); size++)
    {
        VERIFY_IS_FALSE(DeserializePlacement(c_goldenFullBlob, size, decoded));
    }

    // Truncating the body while leaving the header claiming the truncated size
    // must still fail, because the last TLV no longer fits.
    for (size_t bodyBytes = 1; bodyBytes < ARRAYSIZE(c_goldenFullBlob) - c_headerSize; bodyBytes++)
    {
        std::vector<uint8_t> body(
            c_goldenFullBlob + c_headerSize,
            c_goldenFullBlob + c_headerSize + bodyBytes);
        const std::vector<uint8_t> blob = WrapBody(body);

        // Some truncations land exactly on a TLV boundary. Those are only valid
        // if all three required tags survived, which cannot happen before the
        // DPI tag is complete.
        if (bodyBytes < 60)
        {
            VERIFY_IS_FALSE(Deserialize(blob, decoded));
        }
    }
}

void WindowPlacementBlobUnitTests::RejectsOversizedBlob()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body = MakeRequiredBody();

    // Pad with a large unknown tag so the blob exceeds the 4 KiB cap.
    AppendTag(body, static_cast<Tag>(0x7FFF), 0xFFFF);
    body.resize(body.size() + 0xFFFF, 0);

    const std::vector<uint8_t> blob = WrapBody(body);
    VERIFY_IS_GREATER_THAN(blob.size(), c_maxBlobSize);
    VERIFY_IS_FALSE(Deserialize(blob, decoded));

    // Exactly at the cap is still allowed, provided the contents are valid.
    std::vector<uint8_t> atCap = MakeRequiredBody();
    const size_t padding = c_maxBlobSize - c_headerSize - atCap.size() - 4;
    AppendTag(atCap, static_cast<Tag>(0x7FFF), static_cast<uint16_t>(padding));
    atCap.resize(atCap.size() + padding, 0);

    const std::vector<uint8_t> capBlob = WrapBody(atCap);
    VERIFY_ARE_EQUAL(c_maxBlobSize, capBlob.size());
    VERIFY_IS_TRUE(Deserialize(capBlob, decoded));
}

void WindowPlacementBlobUnitTests::RejectsMissingRequiredTags()
{
    PlacementBlobData decoded;

    // No normal rect.
    {
        std::vector<uint8_t> body;
        AppendTag(body, Tag::WorkArea, 16);
        AppendRect(body, MakeRect(0, 0, 1920, 1080));
        AppendTag(body, Tag::Dpi, 4);
        AppendU32(body, 96);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // No work area.
    {
        std::vector<uint8_t> body;
        AppendTag(body, Tag::NormalRect, 16);
        AppendRect(body, MakeRect(100, 200, 900, 800));
        AppendTag(body, Tag::Dpi, 4);
        AppendU32(body, 96);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // No DPI.
    {
        std::vector<uint8_t> body;
        AppendTag(body, Tag::NormalRect, 16);
        AppendRect(body, MakeRect(100, 200, 900, 800));
        AppendTag(body, Tag::WorkArea, 16);
        AppendRect(body, MakeRect(0, 0, 1920, 1080));
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Header only.
    {
        const std::vector<uint8_t> body;
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // All three present is the control case.
    VERIFY_IS_TRUE(Deserialize(WrapBody(MakeRequiredBody()), decoded));
}

void WindowPlacementBlobUnitTests::RejectsWrongFixedTagLength()
{
    PlacementBlobData decoded;

    // A rect tag with 8 bytes instead of 16.
    {
        std::vector<uint8_t> body;
        AppendTag(body, Tag::NormalRect, 8);
        AppendU32(body, 100);
        AppendU32(body, 200);
        AppendTag(body, Tag::WorkArea, 16);
        AppendRect(body, MakeRect(0, 0, 1920, 1080));
        AppendTag(body, Tag::Dpi, 4);
        AppendU32(body, 96);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // DPI with 2 bytes instead of 4.
    {
        std::vector<uint8_t> body;
        AppendTag(body, Tag::NormalRect, 16);
        AppendRect(body, MakeRect(100, 200, 900, 800));
        AppendTag(body, Tag::WorkArea, 16);
        AppendRect(body, MakeRect(0, 0, 1920, 1080));
        AppendTag(body, Tag::Dpi, 2);
        AppendU16(body, 96);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Virtual desktop id with 8 bytes instead of 16.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        AppendTag(body, Tag::VirtualDesktopId, 8);
        AppendU32(body, 0x12345678);
        AppendU32(body, 0x9ABCDEF0);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Show command with 0 bytes.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        AppendTag(body, Tag::ShowCmd, 0);
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }
}

void WindowPlacementBlobUnitTests::RejectsInvalidRects()
{
    PlacementBlobData decoded;

    const RECT invalidRects[] = {
        MakeRect(0, 0, 0, 0),                                       // empty
        MakeRect(100, 100, 100, 200),                               // zero width
        MakeRect(100, 100, 200, 100),                               // zero height
        MakeRect(200, 100, 100, 200),                               // inverted horizontally
        MakeRect(100, 200, 200, 100),                               // inverted vertically
        MakeRect(0, 0, c_maxCoordinate + 1, 100),                   // right out of range
        MakeRect(-c_maxCoordinate - 1, 0, 100, 100),                // left out of range
        MakeRect(0, -c_maxCoordinate - 1, 100, 100),                // top out of range
        MakeRect(0, 0, 100, c_maxCoordinate + 1),                   // bottom out of range
    };

    for (const RECT& rect : invalidRects)
    {
        // As the normal rect.
        {
            std::vector<uint8_t> body;
            AppendTag(body, Tag::NormalRect, 16);
            AppendRect(body, rect);
            AppendTag(body, Tag::WorkArea, 16);
            AppendRect(body, MakeRect(0, 0, 1920, 1080));
            AppendTag(body, Tag::Dpi, 4);
            AppendU32(body, 96);
            VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
        }

        // As the work area.
        {
            std::vector<uint8_t> body;
            AppendTag(body, Tag::NormalRect, 16);
            AppendRect(body, MakeRect(100, 200, 900, 800));
            AppendTag(body, Tag::WorkArea, 16);
            AppendRect(body, rect);
            AppendTag(body, Tag::Dpi, 4);
            AppendU32(body, 96);
            VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
        }

        // As the arrange rect.
        {
            std::vector<uint8_t> body = MakeRequiredBody();
            AppendTag(body, Tag::ArrangeRect, 16);
            AppendRect(body, rect);
            VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
        }
    }
}

void WindowPlacementBlobUnitTests::RejectsInvalidDpi()
{
    PlacementBlobData decoded;

    const uint32_t invalidDpis[] = { 0, 1, 95, c_maxDpi + 1, 0xFFFFFFFF };
    for (uint32_t dpi : invalidDpis)
    {
        VERIFY_IS_FALSE(Deserialize(WrapBody(MakeRequiredBody(dpi)), decoded));
    }

    const uint32_t validDpis[] = { c_minDpi, 120, 144, 192, 240, c_maxDpi };
    for (uint32_t dpi : validDpis)
    {
        VERIFY_IS_TRUE(Deserialize(WrapBody(MakeRequiredBody(dpi)), decoded));
        VERIFY_ARE_EQUAL(dpi, decoded.dpi);
    }
}

void WindowPlacementBlobUnitTests::RejectsInvalidDeviceName()
{
    PlacementBlobData decoded;

    // Odd length.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        AppendTag(body, Tag::DeviceName, 3);
        body.push_back('A');
        body.push_back(0);
        body.push_back('B');
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Longer than the 62-byte cap.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        AppendTag(body, Tag::DeviceName, static_cast<uint16_t>(c_maxDeviceNameBytes + 2));
        for (size_t i = 0; i < (c_maxDeviceNameBytes + 2) / sizeof(WCHAR); i++)
        {
            AppendU16(body, L'A');
        }
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Embedded NUL.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        AppendTag(body, Tag::DeviceName, 6);
        AppendU16(body, L'A');
        AppendU16(body, 0);
        AppendU16(body, L'B');
        VERIFY_IS_FALSE(Deserialize(WrapBody(body), decoded));
    }

    // Exactly at the cap is accepted.
    {
        std::vector<uint8_t> body = MakeRequiredBody();
        const size_t charCount = c_maxDeviceNameBytes / sizeof(WCHAR);
        AppendTag(body, Tag::DeviceName, static_cast<uint16_t>(c_maxDeviceNameBytes));
        for (size_t i = 0; i < charCount; i++)
        {
            AppendU16(body, L'A');
        }
        VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
        VERIFY_ARE_EQUAL(charCount, wcslen(decoded.deviceName));
    }
}

void WindowPlacementBlobUnitTests::SkipsUnknownTags()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body;

    // Unknown tag before everything.
    AppendTag(body, static_cast<Tag>(0x00FF), 3);
    body.push_back(1);
    body.push_back(2);
    body.push_back(3);

    AppendTag(body, Tag::NormalRect, 16);
    AppendRect(body, MakeRect(100, 200, 900, 800));

    // Unknown tag in the middle, with a length that must be honored exactly.
    AppendTag(body, static_cast<Tag>(0x1234), 5);
    body.insert(body.end(), 5, 0xAB);

    AppendTag(body, Tag::WorkArea, 16);
    AppendRect(body, MakeRect(0, 0, 1920, 1080));
    AppendTag(body, Tag::Dpi, 4);
    AppendU32(body, 144);

    // Unknown zero-length tag at the end.
    AppendTag(body, static_cast<Tag>(0xFFFE), 0);

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VerifyRectsEqual(MakeRect(100, 200, 900, 800), decoded.normalRect);
    VERIFY_ARE_EQUAL(144u, decoded.dpi);
}

void WindowPlacementBlobUnitTests::AcceptsHigherMinorVersion()
{
    PlacementBlobData decoded;

    // Minor 1 with a new additive tag the v1 reader does not know.
    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, static_cast<Tag>(0x0030), 4);
    AppendU32(body, 0xDEADBEEF);

    VERIFY_IS_TRUE(Deserialize(WrapBody(body, 1, 1), decoded));
    VERIFY_ARE_EQUAL(96u, decoded.dpi);

    // A far future minor is still readable.
    VERIFY_IS_TRUE(Deserialize(WrapBody(body, 1, 0xFFFF), decoded));
}

void WindowPlacementBlobUnitTests::DuplicateTagsLastOneWins()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body;
    AppendTag(body, Tag::NormalRect, 16);
    AppendRect(body, MakeRect(1, 2, 3, 4));
    AppendTag(body, Tag::NormalRect, 16);
    AppendRect(body, MakeRect(100, 200, 900, 800));
    AppendTag(body, Tag::WorkArea, 16);
    AppendRect(body, MakeRect(0, 0, 1920, 1080));
    AppendTag(body, Tag::Dpi, 4);
    AppendU32(body, 96);
    AppendTag(body, Tag::Dpi, 4);
    AppendU32(body, 192);

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VerifyRectsEqual(MakeRect(100, 200, 900, 800), decoded.normalRect);
    VERIFY_ARE_EQUAL(192u, decoded.dpi);
}

void WindowPlacementBlobUnitTests::TreatsZeroLengthDeviceNameAsAbsent()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, Tag::DeviceName, 0);

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VERIFY_ARE_EQUAL(L'\0', decoded.deviceName[0]);
}

void WindowPlacementBlobUnitTests::TreatsNullVirtualDesktopIdAsAbsent()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, Tag::VirtualDesktopId, 16);
    body.insert(body.end(), 16, 0);

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VERIFY_IS_FALSE(decoded.hasVirtualDesktopId);

    // The writer likewise refuses to emit a null id.
    PlacementBlobData data = MakeMinimalData();
    data.hasVirtualDesktopId = true;
    data.virtualDesktopId = GUID{};

    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(data, blob));
    VerifyBytesEqual(blob, c_goldenMinimalBlob, ARRAYSIZE(c_goldenMinimalBlob));
}

void WindowPlacementBlobUnitTests::CanonicalizesShowCommand()
{
    // Normal aliases.
    const uint32_t normalAliases[] = {
        SW_SHOWNORMAL, SW_SHOWNOACTIVATE, SW_SHOW, SW_SHOWNA, SW_RESTORE, SW_SHOWDEFAULT
    };
    for (uint32_t showCmd : normalAliases)
    {
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), CanonicalizeShowCommand(showCmd));
    }

    // Maximized aliases. SW_MAXIMIZE and SW_SHOWMAXIMIZED share a value.
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_SHOWMAXIMIZED), CanonicalizeShowCommand(SW_SHOWMAXIMIZED));
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_SHOWMAXIMIZED), CanonicalizeShowCommand(SW_MAXIMIZE));

    // Minimized aliases.
    const uint32_t minimizedAliases[] = {
        SW_SHOWMINIMIZED, SW_MINIMIZE, SW_SHOWMINNOACTIVE, SW_FORCEMINIMIZE
    };
    for (uint32_t showCmd : minimizedAliases)
    {
        VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_SHOWMINIMIZED), CanonicalizeShowCommand(showCmd));
    }

    // Hidden and unknown fall back to normal, matching an absent tag.
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), CanonicalizeShowCommand(SW_HIDE));
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), CanonicalizeShowCommand(999));
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), CanonicalizeShowCommand(0xFFFFFFFF));

    // An absent tag also produces normal.
    PlacementBlobData decoded;
    VERIFY_IS_TRUE(Deserialize(WrapBody(MakeRequiredBody()), decoded));
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), decoded.showCmd);

    // SW_HIDE on the wire decodes to normal rather than rejecting.
    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, Tag::ShowCmd, 4);
    AppendU32(body, SW_HIDE);
    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VERIFY_ARE_EQUAL(static_cast<uint32_t>(SW_NORMAL), decoded.showCmd);
}

void WindowPlacementBlobUnitTests::DropsUnknownFlagBits()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, Tag::Flags, 4);

    // Every known bit, plus the reserved 0x0010 bit and a batch of future bits.
    AppendU32(body, c_knownDurableFlags | 0x0010 | 0xFFFF0000);

    // The arrange-dependent flags need a valid arrange rect to survive.
    AppendTag(body, Tag::ArrangeRect, 16);
    AppendRect(body, MakeRect(110, 210, 890, 790));

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VERIFY_ARE_EQUAL(c_knownDurableFlags, static_cast<uint32_t>(decoded.flags));

    // The writer likewise masks unknown bits rather than emitting them.
    PlacementBlobData data = MakeMinimalData();
    data.flags = static_cast<DurableFlags>(0xFFFF0000 | 0x0010);

    std::vector<uint8_t> blob;
    VERIFY_IS_TRUE(SerializePlacement(data, blob));
    VerifyBytesEqual(blob, c_goldenMinimalBlob, ARRAYSIZE(c_goldenMinimalBlob));
}

void WindowPlacementBlobUnitTests::DropsArrangedFlagsWithoutArrangeRect()
{
    PlacementBlobData decoded;

    std::vector<uint8_t> body = MakeRequiredBody();
    AppendTag(body, Tag::Flags, 4);
    AppendU32(body,
        static_cast<uint32_t>(DurableFlags::Arranged) |
        static_cast<uint32_t>(DurableFlags::RestoreToArranged) |
        static_cast<uint32_t>(DurableFlags::AllowSizing));

    VERIFY_IS_TRUE(Deserialize(WrapBody(body), decoded));
    VERIFY_IS_FALSE(decoded.hasArrangeRect);
    VERIFY_IS_FALSE(HasFlag(decoded.flags, DurableFlags::Arranged));
    VERIFY_IS_FALSE(HasFlag(decoded.flags, DurableFlags::RestoreToArranged));

    // Unrelated flags survive.
    VERIFY_IS_TRUE(HasFlag(decoded.flags, DurableFlags::AllowSizing));
}

void WindowPlacementBlobUnitTests::WriterRejectsUnwritablePlacement()
{
    std::vector<uint8_t> blob;

    // Invalid normal rect.
    {
        PlacementBlobData data = MakeMinimalData();
        data.normalRect = MakeRect(0, 0, 0, 0);
        VERIFY_IS_FALSE(SerializePlacement(data, blob));
    }

    // Invalid work area.
    {
        PlacementBlobData data = MakeMinimalData();
        data.workArea = MakeRect(500, 500, 100, 100);
        VERIFY_IS_FALSE(SerializePlacement(data, blob));
    }

    // Out-of-range DPI.
    {
        PlacementBlobData data = MakeMinimalData();
        data.dpi = 0;
        VERIFY_IS_FALSE(SerializePlacement(data, blob));

        data.dpi = c_maxDpi + 1;
        VERIFY_IS_FALSE(SerializePlacement(data, blob));
    }

    // Arranged without an arrange rect.
    {
        PlacementBlobData data = MakeMinimalData();
        data.flags = DurableFlags::Arranged;
        VERIFY_IS_FALSE(SerializePlacement(data, blob));
    }

    // RestoreToArranged with an invalid arrange rect.
    {
        PlacementBlobData data = MakeMinimalData();
        data.flags = DurableFlags::RestoreToArranged;
        data.hasArrangeRect = true;
        data.arrangeRect = MakeRect(0, 0, 0, 0);
        VERIFY_IS_FALSE(SerializePlacement(data, blob));
    }

    // An arrange rect without the flags is simply not written.
    {
        PlacementBlobData data = MakeMinimalData();
        data.hasArrangeRect = true;
        data.arrangeRect = MakeRect(110, 210, 890, 790);
        VERIFY_IS_TRUE(SerializePlacement(data, blob));
        VerifyBytesEqual(blob, c_goldenMinimalBlob, ARRAYSIZE(c_goldenMinimalBlob));
    }
}

void WindowPlacementBlobUnitTests::RejectsGarbageBase64()
{
    PlacementBlobData decoded;

    // Not base64 at all.
    VERIFY_IS_FALSE(TryDecodePlacement(L"not base64 !!!", 14, decoded));

    // Empty.
    VERIFY_IS_FALSE(TryDecodePlacement(L"", 0, decoded));
    VERIFY_IS_FALSE(TryDecodePlacement(nullptr, 0, decoded));

    // Valid base64, but not a placement blob.
    VERIFY_IS_FALSE(TryDecodePlacement(L"AAAAAAAAAAAAAAAA", 16, decoded));

    // Valid base64 of a valid blob with one byte flipped in the magic.
    std::vector<uint8_t> corrupt(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    corrupt[0] = 'X';

    std::wstring text;
    VERIFY_IS_TRUE(Base64Encode(corrupt, text));
    VERIFY_IS_FALSE(TryDecodePlacement(text.c_str(), text.size(), decoded));

    // The uncorrupted blob is the control case.
    std::vector<uint8_t> good(c_goldenMinimalBlob, c_goldenMinimalBlob + ARRAYSIZE(c_goldenMinimalBlob));
    VERIFY_IS_TRUE(Base64Encode(good, text));
    VERIFY_IS_TRUE(TryDecodePlacement(text.c_str(), text.size(), decoded));
}

// ===========================================================================
// Value-name tests
// ===========================================================================

void WindowPlacementValueNameUnitTests::GoldenValueName()
{
    // The worked example from the design doc. The hash is SHA-256 over the
    // UTF-16LE bytes of the raw id with no trailing NUL, Base32-encoded.
    const std::wstring rawId = L"DocumentWindow:doc42";

    std::wstring valueName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(rawId.c_str(), rawId.size(), valueName));

    VERIFY_ARE_EQUAL(
        std::wstring(L"wp1_DocumentWindowdo_5Z6JCUH7DMIYL5L4DSZYMQN7DVDP53MAQZQVFZ4XNQHY5QAFPTTA"),
        valueName);
}

void WindowPlacementValueNameUnitTests::SlugKeepsOnlyAsciiAlphanumerics()
{
    VERIFY_ARE_EQUAL(std::wstring(L"ABCDEFGHIJKLMN"), MakeSlug(L"AB!CD@EF#GH$IJ%KL^MN", 20));
    VERIFY_ARE_EQUAL(std::wstring(L"MainWindow"), MakeSlug(L"Main Window", 11));
    VERIFY_ARE_EQUAL(std::wstring(L"doc42"), MakeSlug(L"doc-42", 6));

    // Non-ASCII characters are dropped, not transliterated. The literal is split
    // so each \x escape ends at the string boundary rather than swallowing the
    // following character as another hex digit.
    VERIFY_ARE_EQUAL(std::wstring(L"abc"), MakeSlug(L"a\x00E9" L"\x4E2D" L"b\x0301" L"c", 6));

    // Digits are kept.
    VERIFY_ARE_EQUAL(std::wstring(L"0123456789"), MakeSlug(L"0-1-2-3-4-5-6-7-8-9", 19));
}

void WindowPlacementValueNameUnitTests::SlugStopsAfterSixteenCharacters()
{
    // 26 accepted characters truncate to the first 16.
    VERIFY_ARE_EQUAL(
        std::wstring(L"ABCDEFGHIJKLMNOP"),
        MakeSlug(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26));

    VERIFY_ARE_EQUAL(c_maxSlugLength, MakeSlug(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26).size());

    // Skipped characters do not count toward the limit.
    VERIFY_ARE_EQUAL(
        std::wstring(L"ABCDEFGHIJKLMNOP"),
        MakeSlug(L"A-B-C-D-E-F-G-H-I-J-K-L-M-N-O-P-Q-R", 35));

    // Exactly 16 accepted characters is not truncated further.
    VERIFY_ARE_EQUAL(std::wstring(L"ABCDEFGHIJKLMNOP"), MakeSlug(L"ABCDEFGHIJKLMNOP", 16));
}

void WindowPlacementValueNameUnitTests::SlugFallsBackWhenNothingAccepted()
{
    VERIFY_ARE_EQUAL(std::wstring(L"id"), MakeSlug(L"::::", 4));
    VERIFY_ARE_EQUAL(std::wstring(L"id"), MakeSlug(L"   ", 3));
    VERIFY_ARE_EQUAL(std::wstring(L"id"), MakeSlug(L"\x4E2D\x6587", 2));
    VERIFY_ARE_EQUAL(std::wstring(L"id"), MakeSlug(L"", 0));
    VERIFY_ARE_EQUAL(std::wstring(L"id"), MakeSlug(nullptr, 0));

    std::wstring valueName;
    const std::wstring rawId = L"::::";
    VERIFY_IS_TRUE(TryGetPlacementValueName(rawId.c_str(), rawId.size(), valueName));
    VERIFY_ARE_EQUAL(
        std::wstring(L"wp1_id_NNWG6L4M5RKZFGCR2DCBQZ56IYU4XKAKC2DW2MVLN37EOJXEP4AQ"),
        valueName);
}

void WindowPlacementValueNameUnitTests::Sha256MatchesKnownVector()
{
    // SHA-256 of the UTF-16LE bytes of L"DocumentWindow:doc42", computed
    // independently of this code.
    const uint8_t expected[c_hashByteCount] = {
        0xEE, 0x7C, 0x91, 0x50, 0xFF, 0x1B, 0x11, 0x85,
        0xF5, 0x7C, 0x1C, 0xB3, 0x86, 0x41, 0xBF, 0x1D,
        0x46, 0xFE, 0xED, 0x80, 0x86, 0x61, 0x52, 0xE7,
        0x97, 0x6C, 0x0F, 0x8E, 0xC0, 0x05, 0x7C, 0xE6,
    };

    const std::wstring rawId = L"DocumentWindow:doc42";

    uint8_t actual[c_hashByteCount] = {};
    VERIFY_IS_TRUE(ComputeSha256(rawId.c_str(), rawId.size(), actual));

    for (size_t i = 0; i < c_hashByteCount; i++)
    {
        VERIFY_ARE_EQUAL(expected[i], actual[i]);
    }

    // The trailing NUL must not be included. Hashing one extra code unit must
    // produce a different digest.
    uint8_t withNul[c_hashByteCount] = {};
    VERIFY_IS_TRUE(ComputeSha256(rawId.c_str(), rawId.size() + 1, withNul));
    VERIFY_IS_TRUE(memcmp(actual, withNul, c_hashByteCount) != 0);

    // Degenerate inputs.
    VERIFY_IS_FALSE(ComputeSha256(nullptr, 4, actual));
    VERIFY_IS_FALSE(ComputeSha256(rawId.c_str(), 0, actual));
    VERIFY_IS_FALSE(ComputeSha256(rawId.c_str(), rawId.size(), nullptr));
}

void WindowPlacementValueNameUnitTests::Base32MatchesRfc4648Vectors()
{
    struct Vector { const char* input; const wchar_t* expected; };

    // RFC 4648 section 10 test vectors, with the '=' padding removed.
    const Vector vectors[] = {
        { "f",      L"MY" },
        { "fo",     L"MZXQ" },
        { "foo",    L"MZXW6" },
        { "foob",   L"MZXW6YQ" },
        { "fooba",  L"MZXW6YTB" },
        { "foobar", L"MZXW6YTBOI" },
    };

    for (const Vector& vector : vectors)
    {
        const size_t length = strlen(vector.input);
        const std::wstring actual = Base32Encode(
            reinterpret_cast<const uint8_t*>(vector.input), length);
        VERIFY_ARE_EQUAL(std::wstring(vector.expected), actual);
    }

    // Empty input produces an empty string rather than failing.
    VERIFY_IS_TRUE(Base32Encode(nullptr, 0).empty());

    // A 32-byte digest always produces exactly 52 characters.
    uint8_t digest[c_hashByteCount] = {};
    VERIFY_ARE_EQUAL(c_base32HashLength, Base32Encode(digest, c_hashByteCount).size());

    memset(digest, 0xFF, sizeof(digest));
    VERIFY_ARE_EQUAL(c_base32HashLength, Base32Encode(digest, c_hashByteCount).size());
}

void WindowPlacementValueNameUnitTests::ValueNameIsStable()
{
    const std::wstring rawId = L"MainWindow";

    std::wstring first;
    std::wstring second;
    VERIFY_IS_TRUE(TryGetPlacementValueName(rawId.c_str(), rawId.size(), first));
    VERIFY_IS_TRUE(TryGetPlacementValueName(rawId.c_str(), rawId.size(), second));

    VERIFY_ARE_EQUAL(first, second);
    VERIFY_ARE_EQUAL(
        std::wstring(L"wp1_MainWindow_WB75XK3P3EI6TOSVS4IG3WMQ6VTA54RUHOLRSYYZP5VUTLYGKZHA"),
        first);
}

void WindowPlacementValueNameUnitTests::DistinctIdsWithSameSlugDiffer()
{
    // Both slug to "DocumentWindowdo" but must not share a value name.
    const std::wstring first = L"DocumentWindow:doc42";
    const std::wstring second = L"DocumentWindow:doc43";

    std::wstring firstName;
    std::wstring secondName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(first.c_str(), first.size(), firstName));
    VERIFY_IS_TRUE(TryGetPlacementValueName(second.c_str(), second.size(), secondName));

    VERIFY_ARE_NOT_EQUAL(firstName, secondName);

    // The slug portion is identical, so only the hash distinguishes them.
    VERIFY_IS_TRUE(firstName.compare(0, 21, L"wp1_DocumentWindowdo_") == 0);
    VERIFY_IS_TRUE(secondName.compare(0, 21, L"wp1_DocumentWindowdo_") == 0);
}

void WindowPlacementValueNameUnitTests::IdsDifferingOnlyByCaseDiffer()
{
    // LocalSettings value names are compared case-insensitively, which is why
    // the hash is Base32 rather than base64. Two ids differing only by case must
    // still produce names that differ in more than case.
    const std::wstring upper = L"MainWindow";
    const std::wstring lower = L"mainwindow";

    std::wstring upperName;
    std::wstring lowerName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(upper.c_str(), upper.size(), upperName));
    VERIFY_IS_TRUE(TryGetPlacementValueName(lower.c_str(), lower.size(), lowerName));

    VERIFY_ARE_NOT_EQUAL(upperName, lowerName);
    VERIFY_ARE_NOT_EQUAL(0, _wcsicmp(upperName.c_str(), lowerName.c_str()));

    VERIFY_ARE_EQUAL(
        std::wstring(L"wp1_mainwindow_LU336YUZIASKGPV64ALYLLUCLWPUPALGWSTMXJOLETSEOBNPX3MA"),
        lowerName);
}

void WindowPlacementValueNameUnitTests::RejectsEmptyId()
{
    std::wstring valueName = L"unchanged";

    VERIFY_IS_FALSE(TryGetPlacementValueName(nullptr, 0, valueName));
    VERIFY_IS_TRUE(valueName.empty());

    valueName = L"unchanged";
    VERIFY_IS_FALSE(TryGetPlacementValueName(L"", 0, valueName));
    VERIFY_IS_TRUE(valueName.empty());

    valueName = L"unchanged";
    VERIFY_IS_FALSE(TryGetPlacementValueName(nullptr, 10, valueName));
    VERIFY_IS_TRUE(valueName.empty());
}

void WindowPlacementValueNameUnitTests::HandlesLongAndUnicodeIds()
{
    // A long id still produces a bounded value name.
    const std::wstring longId(4096, L'A');

    std::wstring valueName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(longId.c_str(), longId.size(), valueName));

    const size_t expectedLength =
        wcslen(c_valueNamePrefix) + c_maxSlugLength + 1 + c_base32HashLength;
    VERIFY_ARE_EQUAL(expectedLength, valueName.size());

    // Surrogate pairs are hashed as their raw UTF-16 code units and contribute
    // nothing to the slug.
    const std::wstring emojiId = L"\xD83D\xDE00 window";
    std::wstring emojiName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(emojiId.c_str(), emojiId.size(), emojiName));
    VERIFY_ARE_EQUAL(std::wstring(L"wp1_window_"), emojiName.substr(0, 11));
    VERIFY_ARE_EQUAL(wcslen(c_valueNamePrefix) + 6 + 1 + c_base32HashLength, emojiName.size());

    // An id made only of surrogate pairs falls back to the default slug but
    // still hashes successfully.
    const std::wstring onlyEmoji = L"\xD83D\xDE00\xD83D\xDE01";
    std::wstring onlyEmojiName;
    VERIFY_IS_TRUE(TryGetPlacementValueName(onlyEmoji.c_str(), onlyEmoji.size(), onlyEmojiName));
    VERIFY_ARE_EQUAL(std::wstring(L"wp1_id_"), onlyEmojiName.substr(0, 7));
}

} } } } }
