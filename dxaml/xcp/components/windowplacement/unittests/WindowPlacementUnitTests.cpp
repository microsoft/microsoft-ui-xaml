// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowPlacementUnitTests.h"

#include <windows.h>
#include <vector>
#include <string>

#include <WindowPlacementData.h>
#include <WindowPlacementFormat.h>
#include <WindowPlacementSerializer.h>
#include <WindowPlacementValueName.h>

using namespace WindowPlacement;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace WindowPlacement {

    namespace
    {
        WindowPlacementData MakeMinimalPlacement()
        {
            WindowPlacementData data;
            data.hasNormalRect = true;
            data.normalRect = { 100, 200, 900, 800 };
            return data;
        }

        WindowPlacementData MakeFullPlacement()
        {
            WindowPlacementData data;
            data.hasNormalRect = true;
            data.normalRect = { 10, 20, 810, 620 };
            data.hasWorkArea = true;
            data.workArea = { 0, 0, 1920, 1040 };
            data.hasArrangeRect = true;
            data.arrangeRect = { -7, 0, 967, 1040 };
            data.hasDpi = true;
            data.dpi = 144;
            data.hasShowCmd = true;
            data.showCmd = SW_SHOWMAXIMIZED;
            data.hasFlags = true;
            data.flags = WindowPlacementFlags_RestoreToMaximized | WindowPlacementFlags_AllowSizing;
            data.hasDeviceName = true;
            data.deviceName = L"\\\\.\\DISPLAY2";
            data.hasVirtualDesktopId = true;
            data.virtualDesktopId = { 0x11223344, 0x5566, 0x7788,
                { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 } };
            return data;
        }

        void VerifyRectEqual(const WindowPlacementRect& a, const WindowPlacementRect& b)
        {
            VERIFY_ARE_EQUAL(a.left, b.left);
            VERIFY_ARE_EQUAL(a.top, b.top);
            VERIFY_ARE_EQUAL(a.right, b.right);
            VERIFY_ARE_EQUAL(a.bottom, b.bottom);
        }

        bool IsBase32Char(wchar_t ch)
        {
            return (ch >= L'A' && ch <= L'Z') || (ch >= L'2' && ch <= L'7');
        }
    }

    void WindowPlacementUnitTests::RoundTripMinimal()
    {
        const WindowPlacementData original = MakeMinimalPlacement();
        const std::vector<uint8_t> blob = SerializePlacement(original);
        VERIFY_IS_GREATER_THAN(blob.size(), c_headerLength);

        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_IS_TRUE(parsed.hasNormalRect);
        VerifyRectEqual(parsed.normalRect, original.normalRect);
        VERIFY_IS_FALSE(parsed.hasWorkArea);
        VERIFY_IS_FALSE(parsed.hasDpi);
        VERIFY_IS_FALSE(parsed.hasShowCmd);
        VERIFY_IS_FALSE(parsed.hasFlags);
        VERIFY_IS_FALSE(parsed.hasDeviceName);
        VERIFY_IS_FALSE(parsed.hasVirtualDesktopId);
    }

    void WindowPlacementUnitTests::RoundTripAllFields()
    {
        const WindowPlacementData original = MakeFullPlacement();
        const std::vector<uint8_t> blob = SerializePlacement(original);

        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));

        VerifyRectEqual(parsed.normalRect, original.normalRect);
        VERIFY_IS_TRUE(parsed.hasWorkArea);
        VerifyRectEqual(parsed.workArea, original.workArea);
        VERIFY_IS_TRUE(parsed.hasArrangeRect);
        VerifyRectEqual(parsed.arrangeRect, original.arrangeRect);
        VERIFY_IS_TRUE(parsed.hasDpi);
        VERIFY_ARE_EQUAL(parsed.dpi, original.dpi);
        VERIFY_IS_TRUE(parsed.hasShowCmd);
        VERIFY_ARE_EQUAL(parsed.showCmd, static_cast<uint32_t>(SW_SHOWMAXIMIZED));
        VERIFY_IS_TRUE(parsed.hasFlags);
        VERIFY_ARE_EQUAL(parsed.flags, original.flags);
        VERIFY_IS_TRUE(parsed.hasDeviceName);
        VERIFY_IS_TRUE(parsed.deviceName == original.deviceName);
        VERIFY_IS_TRUE(parsed.hasVirtualDesktopId);
        VERIFY_ARE_EQUAL(parsed.virtualDesktopId.Data1, original.virtualDesktopId.Data1);
        VERIFY_ARE_EQUAL(parsed.virtualDesktopId.Data2, original.virtualDesktopId.Data2);
        VERIFY_ARE_EQUAL(parsed.virtualDesktopId.Data3, original.virtualDesktopId.Data3);
        for (int i = 0; i < 8; ++i)
        {
            VERIFY_ARE_EQUAL(parsed.virtualDesktopId.Data4[i], original.virtualDesktopId.Data4[i]);
        }
    }

    void WindowPlacementUnitTests::Base64RoundTrip()
    {
        const WindowPlacementData original = MakeFullPlacement();
        const std::wstring encoded = SerializePlacementToBase64(original);
        VERIFY_IS_FALSE(encoded.empty());

        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacementFromBase64(encoded, parsed));
        VerifyRectEqual(parsed.normalRect, original.normalRect);
        VERIFY_ARE_EQUAL(parsed.dpi, original.dpi);
    }

    void WindowPlacementUnitTests::GoldenBlobV1()
    {
        // A pinned canonical placement: normal rect + dpi + maximized show command + the
        // RestoreToMaximized flag. If this array must change, the wire format changed and the
        // major/minor version and compatibility story must be reviewed.
        WindowPlacementData data;
        data.hasNormalRect = true;
        data.normalRect = { 100, 200, 900, 800 };
        data.hasDpi = true;
        data.dpi = 96;
        data.hasShowCmd = true;
        data.showCmd = SW_SHOWMAXIMIZED; // 3
        data.hasFlags = true;
        data.flags = WindowPlacementFlags_RestoreToMaximized; // 0x0001

        const std::vector<uint8_t> expected = {
            // Header: "WPL1", major=1, minor=0, totalLength=56
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00,
            // TAG_NORMAL_RECT (0x0001), len 16, {100, 200, 900, 800}
            0x01, 0x00, 0x10, 0x00,
            0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
            // TAG_DPI (0x0010), len 4, value 96
            0x10, 0x00, 0x04, 0x00, 0x60, 0x00, 0x00, 0x00,
            // TAG_SHOW_CMD (0x0011), len 4, value 3
            0x11, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00,
            // TAG_FLAGS (0x0012), len 4, value 1
            0x12, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
        };

        const std::vector<uint8_t> actual = SerializePlacement(data);
        VERIFY_ARE_EQUAL(actual.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i)
        {
            VERIFY_ARE_EQUAL(actual[i], expected[i], WEX::Common::String().Format(L"byte %u", static_cast<unsigned>(i)));
        }

        // And it parses back to the same values.
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(actual, parsed));
        VERIFY_ARE_EQUAL(parsed.dpi, 96u);
        VERIFY_ARE_EQUAL(parsed.showCmd, static_cast<uint32_t>(SW_SHOWMAXIMIZED));
        VERIFY_ARE_EQUAL(parsed.flags, static_cast<uint32_t>(WindowPlacementFlags_RestoreToMaximized));
    }

    void WindowPlacementUnitTests::WriterIsCanonicalRegardlessOfSetOrder()
    {
        WindowPlacementData a;
        a.hasFlags = true; a.flags = WindowPlacementFlags_Arranged;
        a.hasNormalRect = true; a.normalRect = { 1, 2, 3, 4 };
        a.hasDpi = true; a.dpi = 120;

        WindowPlacementData b;
        b.hasNormalRect = true; b.normalRect = { 1, 2, 3, 4 };
        b.hasDpi = true; b.dpi = 120;
        b.hasFlags = true; b.flags = WindowPlacementFlags_Arranged;

        VERIFY_IS_TRUE(SerializePlacement(a) == SerializePlacement(b));
    }

    void WindowPlacementUnitTests::SerializeEmptyWithoutNormalRect()
    {
        WindowPlacementData data;
        data.hasDpi = true;
        data.dpi = 96;
        VERIFY_IS_TRUE(SerializePlacement(data).empty());
        VERIFY_IS_TRUE(SerializePlacementToBase64(data).empty());
    }

    void WindowPlacementUnitTests::RejectMissingNormalRect()
    {
        // Hand-build a blob whose only field is a DPI tag (no normal rect).
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, // total 20
            0x10, 0x00, 0x04, 0x00, 0x60, 0x00, 0x00, 0x00,
        };
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectBadMagic()
    {
        std::vector<uint8_t> blob = SerializePlacement(MakeMinimalPlacement());
        blob[0] = 'X';
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectUnknownMajor()
    {
        std::vector<uint8_t> blob = SerializePlacement(MakeMinimalPlacement());
        blob[4] = 0x02; // major = 2
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::ToleratesUnknownMinor()
    {
        std::vector<uint8_t> blob = SerializePlacement(MakeMinimalPlacement());
        blob[6] = 0x09; // minor = 9
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_IS_TRUE(parsed.hasNormalRect);
    }

    void WindowPlacementUnitTests::RejectWrongTotalLength()
    {
        std::vector<uint8_t> blob = SerializePlacement(MakeMinimalPlacement());
        blob[8] = static_cast<uint8_t>(blob[8] + 1); // total length no longer matches
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectTruncatedField()
    {
        // Header claims a total length that lands in the middle of the normal-rect value.
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, // total 18
            0x01, 0x00, 0x10, 0x00, 0x64, 0x00, // field claims 16 bytes but only 2 remain
        };
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectWrongFixedWidthLength()
    {
        // A normal-rect tag with a 12-byte value must reject the whole blob.
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, // total 28
            0x01, 0x00, 0x0C, 0x00,
            0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00,
        };
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectOutOfRangeCoordinate()
    {
        WindowPlacementData data = MakeMinimalPlacement();
        data.normalRect.right = c_maxCoordinateMagnitude + 1;
        // The serializer still writes it, but the parser must treat it as corruption.
        const std::vector<uint8_t> blob = SerializePlacement(data);
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectOddDeviceNameLength()
    {
        // Device-name value length of 3 (odd) must reject the blob.
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, // total 39
            // normal rect
            0x01, 0x00, 0x10, 0x00,
            0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
            // device name, odd length 3
            0x20, 0x00, 0x03, 0x00, 0x41, 0x00, 0x42,
        };
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectOversizeDeviceName()
    {
        // Device-name value length of 64 exceeds the 62-byte cap and must reject the blob.
        std::vector<uint8_t> blob;
        auto appendU16 = [&](uint16_t v) { blob.push_back(v & 0xFF); blob.push_back((v >> 8) & 0xFF); };
        auto appendU32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) blob.push_back((v >> (8 * i)) & 0xFF); };

        // header placeholder
        blob.insert(blob.end(), { 'W', 'P', 'L', '1' });
        appendU16(1); appendU16(0);
        const uint32_t total = 12 + 20 + 4 + 64;
        appendU32(total);
        // normal rect
        appendU16(0x0001); appendU16(16);
        appendU32(100); appendU32(200); appendU32(900); appendU32(800);
        // device name, length 64
        appendU16(0x0020); appendU16(64);
        for (int i = 0; i < 64; ++i) blob.push_back(0x41);

        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::RejectOversizeBlob()
    {
        std::vector<uint8_t> blob(c_maxBlobLength + 1, 0);
        blob[0] = 'W'; blob[1] = 'P'; blob[2] = 'L'; blob[3] = '1';
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacement(blob, parsed));
    }

    void WindowPlacementUnitTests::SkipUnknownTag()
    {
        // Normal rect + an unknown tag 0x7FFF with a 2-byte value. The unknown tag is skipped and
        // the placement remains valid.
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, // total 38
            0x01, 0x00, 0x10, 0x00,
            0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
            0xFF, 0x7F, 0x02, 0x00, 0xAB, 0xCD,
        };
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_IS_TRUE(parsed.hasNormalRect);
    }

    void WindowPlacementUnitTests::UnknownShowCmdTreatedAsAbsent()
    {
        WindowPlacementData data = MakeMinimalPlacement();
        data.hasShowCmd = true;
        data.showCmd = 999; // above SW_FORCEMINIMIZE
        // The writer emits it verbatim; the parser drops the unknown value but keeps the rest.
        const std::vector<uint8_t> blob = SerializePlacement(data);
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_IS_TRUE(parsed.hasNormalRect);
        VERIFY_IS_FALSE(parsed.hasShowCmd);
    }

    void WindowPlacementUnitTests::UnknownFlagBitsIgnored()
    {
        WindowPlacementData data = MakeMinimalPlacement();
        data.hasFlags = true;
        data.flags = WindowPlacementFlags_Arranged | 0x8000; // one known bit + one unknown bit
        const std::vector<uint8_t> blob = SerializePlacement(data);
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_ARE_EQUAL(parsed.flags, static_cast<uint32_t>(WindowPlacementFlags_Arranged));
    }

    void WindowPlacementUnitTests::DuplicateTagLastWins()
    {
        // Two DPI tags; the second value wins.
        std::vector<uint8_t> blob = {
            0x57, 0x50, 0x4C, 0x31, 0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, // total 48
            0x01, 0x00, 0x10, 0x00,
            0x64, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00, 0x00, 0x20, 0x03, 0x00, 0x00,
            0x10, 0x00, 0x04, 0x00, 0x60, 0x00, 0x00, 0x00, // dpi 96
            0x10, 0x00, 0x04, 0x00, 0x90, 0x00, 0x00, 0x00, // dpi 144
        };
        WindowPlacementData parsed;
        VERIFY_IS_TRUE(TryParsePlacement(blob, parsed));
        VERIFY_ARE_EQUAL(parsed.dpi, 144u);
    }

    void WindowPlacementUnitTests::InvalidBase64ReturnsFalse()
    {
        WindowPlacementData parsed;
        VERIFY_IS_FALSE(TryParsePlacementFromBase64(L"not valid base64 @@@@", parsed));
        VERIFY_IS_FALSE(TryParsePlacementFromBase64(L"", parsed));
    }

    void WindowPlacementUnitTests::ValueNameShape()
    {
        const std::wstring name = DerivePlacementValueName(L"DocumentWindow:doc42");
        // wp1_ + slug(16 accepted chars: DocumentWindowdo) + _ + 52-char hash
        VERIFY_IS_TRUE(name.rfind(L"wp1_DocumentWindowdo_", 0) == 0);
        const size_t expectedLength = 4 /*wp1_*/ + 16 /*slug*/ + 1 /*_*/ + c_hashBase32Length;
        VERIFY_ARE_EQUAL(name.size(), expectedLength);

        const std::wstring hash = name.substr(name.size() - c_hashBase32Length);
        for (wchar_t ch : hash)
        {
            VERIFY_IS_TRUE(IsBase32Char(ch));
        }
    }

    void WindowPlacementUnitTests::SlugRules()
    {
        VERIFY_IS_TRUE(MakePlacementSlug(L"MainWindow") == L"MainWindow");
        // Non-alphanumerics dropped.
        VERIFY_IS_TRUE(MakePlacementSlug(L"Doc:42/win") == L"Doc42win");
        // Capped at 16 accepted characters.
        VERIFY_IS_TRUE(MakePlacementSlug(L"ABCDEFGHIJKLMNOPQRSTUV") == L"ABCDEFGHIJKLMNOP");
        // No accepted characters -> "id".
        VERIFY_IS_TRUE(MakePlacementSlug(L":/\\.-") == L"id");
        VERIFY_IS_TRUE(MakePlacementSlug(L"") == L"id");
    }

    void WindowPlacementUnitTests::HashIsDeterministicAndDistinct()
    {
        const std::wstring a1 = MakePlacementHash(L"MainWindow");
        const std::wstring a2 = MakePlacementHash(L"MainWindow");
        const std::wstring b = MakePlacementHash(L"MainWindow2");

        VERIFY_ARE_EQUAL(a1.size(), c_hashBase32Length);
        VERIFY_IS_TRUE(a1 == a2);        // deterministic
        VERIFY_IS_FALSE(a1 == b);        // distinct inputs -> distinct hashes

        // Case matters in the id, so it changes the hash even though value names compare
        // case-insensitively.
        VERIFY_IS_FALSE(MakePlacementHash(L"mainwindow") == a1);
    }

} } } } }
