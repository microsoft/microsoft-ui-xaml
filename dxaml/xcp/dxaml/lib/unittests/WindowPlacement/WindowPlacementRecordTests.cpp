// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementRecord.h"
#include <limits>

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    // Independent little-endian golden: normal (-20,-10,40,30), work (-100,-50,200,100), 96 DPI.
    const std::vector<uint8_t> Golden{
        0x57,0x50,0x4c,0x31, 1,0,0,0, 76,0,0,0,
        1,0,16,0, 0xec,0xff,0xff,0xff, 0xf6,0xff,0xff,0xff, 20,0,0,0, 20,0,0,0,
        2,0,16,0, 0x9c,0xff,0xff,0xff, 0xce,0xff,0xff,0xff, 100,0,0,0, 50,0,0,0,
        16,0,4,0, 96,0,0,0,
        17,0,4,0, 1,0,0,0,
        18,0,4,0, 0,0,0,0
    };

    void Set32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value)
    {
        for (size_t i = 0; i < 4; ++i) bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
    }

    void FixLength(std::vector<uint8_t>& bytes)
    {
        Set32(bytes, 8, static_cast<uint32_t>(bytes.size()));
    }

    void Append(std::vector<uint8_t>& bytes, uint16_t tag, const std::vector<uint8_t>& value)
    {
        bytes.push_back(static_cast<uint8_t>(tag));
        bytes.push_back(static_cast<uint8_t>(tag >> 8));
        bytes.push_back(static_cast<uint8_t>(value.size()));
        bytes.push_back(static_cast<uint8_t>(value.size() >> 8));
        bytes.insert(bytes.end(), value.begin(), value.end());
        FixLength(bytes);
    }

    Record Sample()
    {
        Record record;
        record.Placement.NormalRect = {-20, -10, 40, 30};
        record.Placement.WorkArea = {-100, -50, 200, 100};
        record.Placement.Dpi = 96;
        return record;
    }

    Record Decode(const std::vector<uint8_t>& bytes)
    {
        Record record;
        VERIFY_IS_TRUE(DecodeRecord(bytes.data(), bytes.size(), record) == DecodeResult::Success);
        return record;
    }

    void Reject(const std::vector<uint8_t>& bytes)
    {
        auto output = Sample();
        output.SaveSequence = 42;
        VERIFY_IS_TRUE(DecodeRecord(bytes.data(), bytes.size(), output) == DecodeResult::InvalidData);
        VERIFY_ARE_EQUAL(uint64_t{42}, output.SaveSequence);
        VERIFY_ARE_EQUAL(96, output.Placement.Dpi);
        VERIFY_ARE_EQUAL(-20, output.Placement.NormalRect.X);
    }
}

class WindowPlacementRecordTests
{
public:
    TEST_CLASS(WindowPlacementRecordTests);

    TEST_METHOD(GoldenRecord)
    {
        std::vector<uint8_t> encoded;
        VERIFY_IS_TRUE(EncodeRecord(Sample(), encoded));
        VERIFY_IS_TRUE(encoded == Golden);
        const auto decoded = Decode(Golden);
        VERIFY_ARE_EQUAL(-20, decoded.Placement.NormalRect.X);
        VERIFY_ARE_EQUAL(-10, decoded.Placement.NormalRect.Y);
        VERIFY_ARE_EQUAL(40, decoded.Placement.NormalRect.Width);
        VERIFY_ARE_EQUAL(30, decoded.Placement.NormalRect.Height);
        VERIFY_ARE_EQUAL(-100, decoded.Placement.WorkArea.X);
        VERIFY_ARE_EQUAL(-50, decoded.Placement.WorkArea.Y);
        VERIFY_ARE_EQUAL(200, decoded.Placement.WorkArea.Width);
        VERIFY_ARE_EQUAL(100, decoded.Placement.WorkArea.Height);
        VERIFY_IS_TRUE(decoded.Placement.PlacementState == State::Normal);
        VERIFY_IS_FALSE(decoded.Placement.SnapRect.has_value());
        VERIFY_IS_FALSE(decoded.Placement.VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(decoded.Placement.DisplayDeviceName.empty());
        VERIFY_ARE_EQUAL(uint64_t{0}, decoded.SaveSequence);
    }

    TEST_METHOD(AllSixStates)
    {
        const State states[]{State::Normal, State::Maximized, State::Minimized, State::Snapped,
            State::MinimizedFromMaximized, State::MinimizedFromSnapped};
        const uint8_t commands[]{1, 3, 2, 1, 2, 2};
        const uint8_t flags[]{0, 0, 0, 2, 1, 32};
        for (size_t i = 0; i < 6; ++i)
        {
            auto record = Sample();
            record.Placement.PlacementState = states[i];
            record.Placement.SnapRect = Rect{-100, -50, 100, 100};
            auto expected = Golden;
            Set32(expected, 64, commands[i]);
            Set32(expected, 72, flags[i]);
            const bool hasSnap = i == 3 || i == 5;
            if (hasSnap)
            {
                const std::vector<uint8_t> snap{3,0,16,0, 0x9c,0xff,0xff,0xff,
                    0xce,0xff,0xff,0xff, 0,0,0,0, 50,0,0,0};
                expected.insert(expected.begin() + 52, snap.begin(), snap.end());
                FixLength(expected);
            }
            std::vector<uint8_t> encoded;
            VERIFY_IS_TRUE(EncodeRecord(record, encoded));
            VERIFY_IS_TRUE(encoded == expected);
            const auto decoded = Decode(expected);
            VERIFY_IS_TRUE(decoded.Placement.PlacementState == states[i]);
            VERIFY_ARE_EQUAL(hasSnap, decoded.Placement.SnapRect.has_value());
        }
    }

    TEST_METHOD(OptionalFieldsAndMaximumCanonicalSize)
    {
        auto record = Sample();
        record.Placement.PlacementState = State::MinimizedFromSnapped;
        record.Placement.SnapRect = record.Placement.WorkArea;
        record.Placement.DisplayDeviceName = std::u16string(31, u'\u754c');
        record.Placement.VirtualDesktopId = GUID{0x12345678, 0x9abc, 0xdef0, {1,2,3,4,5,6,7,8}};
        record.SaveSequence = 0x8877665544332211ULL;
        std::vector<uint8_t> encoded;
        VERIFY_IS_TRUE(EncodeRecord(record, encoded));
        VERIFY_ARE_EQUAL(size_t{194}, encoded.size());
        const std::vector<uint8_t> tail{33,0,16,0, 0x78,0x56,0x34,0x12, 0xbc,0x9a,0xf0,0xde,
            1,2,3,4,5,6,7,8, 48,0,8,0, 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        VERIFY_IS_TRUE(std::vector<uint8_t>(encoded.end() - tail.size(), encoded.end()) == tail);
        const auto decoded = Decode(encoded);
        VERIFY_IS_TRUE(decoded.Placement.DisplayDeviceName == record.Placement.DisplayDeviceName);
        VERIFY_IS_TRUE(IsEqualGUID(*decoded.Placement.VirtualDesktopId, *record.Placement.VirtualDesktopId) != 0);
        VERIFY_ARE_EQUAL(record.SaveSequence, decoded.SaveSequence);
    }

    TEST_METHOD(EmptyIdentityAndLegacyDefaults)
    {
        auto record = Sample();
        record.Placement.VirtualDesktopId = GUID{};
        std::vector<uint8_t> encoded;
        VERIFY_IS_TRUE(EncodeRecord(record, encoded));
        VERIFY_IS_TRUE(encoded == Golden);
        encoded.resize(60); // Only required fields; missing state tags default to normal.
        FixLength(encoded);
        Append(encoded, 0x20, {});
        Append(encoded, 0x21, std::vector<uint8_t>(16));
        Append(encoded, 0x30, std::vector<uint8_t>(8));
        const auto decoded = Decode(encoded);
        VERIFY_IS_TRUE(decoded.Placement.PlacementState == State::Normal);
        VERIFY_IS_TRUE(decoded.Placement.DisplayDeviceName.empty());
        VERIFY_IS_FALSE(decoded.Placement.VirtualDesktopId.has_value());
        VERIFY_ARE_EQUAL(uint64_t{0}, decoded.SaveSequence);
    }

    TEST_METHOD(VersionAndUnknownFields)
    {
        auto bytes = Golden;
        bytes[6] = 255;
        bytes[7] = 255;
        Append(bytes, 0xffff, {1,2,3});
        Append(bytes, 0x100, {});
        Decode(bytes);
        bytes[4] = 2;
        auto result = Sample();
        result.SaveSequence = 42;
        VERIFY_IS_TRUE(DecodeRecord(bytes.data(), bytes.size(), result) == DecodeResult::UnsupportedVersion);
        VERIFY_ARE_EQUAL(uint64_t{42}, result.SaveSequence);
        bytes[4] = 0;
        VERIFY_IS_TRUE(DecodeRecord(bytes.data(), bytes.size(), result) == DecodeResult::UnsupportedVersion);
    }

    TEST_METHOD(EnvelopeAndBounds)
    {
        Record result;
        VERIFY_IS_TRUE(DecodeRecord(nullptr, 12, result) == DecodeResult::InvalidData);
        for (size_t size = 0; size < Golden.size(); ++size)
        {
            Reject(std::vector<uint8_t>(Golden.begin(), Golden.begin() + size));
        }
        for (size_t i = 0; i < 4; ++i)
        {
            auto bytes = Golden;
            bytes[i] ^= 1;
            Reject(bytes);
        }
        auto bytes = Golden;
        bytes.push_back(0);
        Reject(bytes); // Trailing data beyond declared length.
        FixLength(bytes);
        Reject(bytes); // Truncated TLV header within declared length.
        bytes = Golden;
        Append(bytes, 0x100, std::vector<uint8_t>(MaximumRecordBytes - Golden.size() - 4));
        Decode(bytes); // The exact ceiling is accepted.
        bytes.push_back(0);
        FixLength(bytes);
        Reject(bytes);
        bytes = Golden;
        bytes[14] = 0xff;
        bytes[15] = 0xff;
        Reject(bytes);
    }

    TEST_METHOD(RequiredFieldsAndKnownLengths)
    {
        const size_t offsets[]{12, 32, 52};
        const size_t lengths[]{20, 20, 8};
        for (size_t i = 0; i < 3; ++i)
        {
            auto bytes = Golden;
            bytes.erase(bytes.begin() + offsets[i], bytes.begin() + offsets[i] + lengths[i]);
            FixLength(bytes);
            Reject(bytes);
        }
        const uint16_t tags[]{1, 2, 3, 16, 17, 18, 33, 48};
        const size_t sizes[]{16, 16, 16, 4, 4, 4, 16, 8};
        for (size_t i = 0; i < 8; ++i)
        {
            for (auto length : {size_t{0}, sizes[i] - 1, sizes[i] + 1})
            {
                auto bytes = Golden;
                Append(bytes, tags[i], std::vector<uint8_t>(length));
                Reject(bytes);
            }
        }
    }

    TEST_METHOD(DuplicateFieldsAndUnorderedTags)
    {
        auto bytes = Golden;
        Append(bytes, 16, {192,0,0,0});
        VERIFY_ARE_EQUAL(192, Decode(bytes).Placement.Dpi);
        // Rotate complete required fields; order is not part of reader validity.
        bytes = std::vector<uint8_t>(Golden.begin(), Golden.begin() + 12);
        bytes.insert(bytes.end(), Golden.begin() + 52, Golden.end());
        bytes.insert(bytes.end(), Golden.begin() + 12, Golden.begin() + 52);
        Decode(bytes);
        bytes = Golden;
        Append(bytes, 16, {95,0,0,0});
        Append(bytes, 16, {96,0,0,0});
        Reject(bytes); // A valid last duplicate must not hide an earlier invalid field.
        bytes = Golden;
        Append(bytes, 3, std::vector<uint8_t>(16));
        bytes.insert(bytes.end(), Golden.begin() + 12, Golden.begin() + 32);
        bytes[bytes.size() - 20] = 3;
        FixLength(bytes);
        Reject(bytes);
    }

    TEST_METHOD(ShowAliasesAndFlagCombinations)
    {
        for (uint32_t command = 0; command <= 12; ++command)
        {
            auto bytes = Golden;
            Set32(bytes, 64, command);
            Set32(bytes, 72, 0xffffffdc); // Unknown/reserved flags never become native policy.
            const auto expected = command == 3 ? State::Maximized :
                (command == 2 || command == 6 || command == 7 || command == 11) ? State::Minimized : State::Normal;
            VERIFY_IS_TRUE(Decode(bytes).Placement.PlacementState == expected);
        }
        auto bytes = Golden;
        Set32(bytes, 64, 0xffffffff);
        VERIFY_IS_TRUE(Decode(bytes).Placement.PlacementState == State::Normal);
        for (auto command : {1u, 2u, 3u})
        {
            for (auto flags : {1u, 2u, 3u, 32u, 33u, 34u, 35u})
            {
                bytes = Golden;
                Set32(bytes, 64, command);
                Set32(bytes, 72, flags);
                if (command == 2 && flags == 1)
                    VERIFY_IS_TRUE(Decode(bytes).Placement.PlacementState == State::MinimizedFromMaximized);
                else
                    Reject(bytes); // Conflicts, incompatible command, or missing required snap.
            }
        }
    }

    TEST_METHOD(RectangleAndDpiValidation)
    {
        auto record = Sample();
        const auto max = (std::numeric_limits<int32_t>::max)();
        const auto min = (std::numeric_limits<int32_t>::min)();
        for (const Rect rect : {Rect{0,0,0,1}, Rect{0,0,1,-1}, Rect{max,0,1,1}, Rect{0,max,1,1}})
        {
            record = Sample(); record.Placement.NormalRect = rect;
            VERIFY_IS_FALSE(IsValid(record.Placement));
            record = Sample(); record.Placement.WorkArea = rect;
            VERIFY_IS_FALSE(IsValid(record.Placement));
            record = Sample(); record.Placement.SnapRect = rect;
            VERIFY_IS_FALSE(IsValid(record.Placement)); // Even when not snapped.
        }
        record = Sample();
        record.Placement.NormalRect = {100,0,1,1}; // Touching edge is not positive-area intersection.
        VERIFY_IS_FALSE(IsValid(record.Placement));
        for (const Rect rect : {Rect{min,min,max,max}, Rect{max-1,max-1,1,1}, Rect{-2000000,0,1,1}})
        {
            record = Sample();
            record.Placement.NormalRect = record.Placement.WorkArea = rect;
            record.Placement.Dpi = max;
            std::vector<uint8_t> bytes;
            VERIFY_IS_TRUE(EncodeRecord(record, bytes));
            VERIFY_ARE_EQUAL(rect.X, Decode(bytes).Placement.NormalRect.X);
        }
        for (auto dpi : {0u, 95u, 0x80000000u, 0xffffffffu})
        {
            auto bytes = Golden;
            Set32(bytes, 56, dpi);
            Reject(bytes);
        }
        auto bytes = Golden;
        Set32(bytes, 16, 0x80000000); Set32(bytes, 24, 0x7fffffff);
        Reject(bytes); // Edges individually fit, but width does not fit RectInt32.
        bytes = Golden;
        Set32(bytes, 24, static_cast<uint32_t>(-20));
        Reject(bytes); // Zero width.
    }

    TEST_METHOD(DeviceNameValidation)
    {
        for (const auto& name : {std::u16string(32, u'a'), std::u16string{u'a', u'\0', u'b'}})
        {
            auto record = Sample();
            record.Placement.DisplayDeviceName = name;
            VERIFY_IS_FALSE(IsValid(record.Placement));
        }
        for (const auto& value : {std::vector<uint8_t>{1}, std::vector<uint8_t>(64, 1), std::vector<uint8_t>{0,0}})
        {
            auto bytes = Golden;
            Append(bytes, 32, value);
            Append(bytes, 32, {65,0});
            Reject(bytes);
        }
        auto bytes = Golden;
        Append(bytes, 32, {0x3d,0xd8,0x00,0xde,0x00,0xd8}); // UTF-16 code units, not normalized Unicode.
        const auto record = Decode(bytes);
        VERIFY_ARE_EQUAL(size_t{3}, record.Placement.DisplayDeviceName.size());
        VERIFY_ARE_EQUAL(char16_t{0xd800}, record.Placement.DisplayDeviceName[2]);
    }

    TEST_METHOD(InvalidEncodeLeavesOutputUnchanged)
    {
        auto record = Sample();
        record.Placement.PlacementState = static_cast<State>(-1);
        auto bytes = Golden;
        VERIFY_IS_FALSE(EncodeRecord(record, bytes));
        VERIFY_IS_TRUE(bytes == Golden);
        record.Placement.PlacementState = State::Snapped;
        VERIFY_IS_FALSE(EncodeRecord(record, bytes));
        VERIFY_IS_TRUE(bytes == Golden);
        record = Sample();
        record.Placement.Dpi = -1;
        VERIFY_IS_FALSE(EncodeRecord(record, bytes));
        VERIFY_IS_TRUE(bytes == Golden);
    }

    TEST_METHOD(MutatedRecordsRemainStructurallyValidOrFail)
    {
        for (size_t offset = 0; offset < Golden.size(); ++offset)
        {
            for (auto value : {uint8_t{0}, uint8_t{0x7f}, uint8_t{0x80}, uint8_t{0xff}})
            {
                auto bytes = Golden;
                bytes[offset] = value;
                Record result;
                if (DecodeRecord(bytes.data(), bytes.size(), result) == DecodeResult::Success)
                {
                    VERIFY_IS_TRUE(IsValid(result.Placement));
                    std::vector<uint8_t> canonical;
                    VERIFY_IS_TRUE(EncodeRecord(result, canonical));
                    Decode(canonical);
                }
            }
        }
    }
};
