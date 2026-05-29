// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XbfSubReaderUnitTests.h"
#include <XamlBinaryFormatSubReader2.h>
#include <XamlQualifiedObject.h>

#include <cstdint>
#include <vector>
#include <cstring>
#include <limits>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    // LEB128 encoder for uint32 test values
    static void EncodeLEB128(std::uint32_t value, std::vector<std::uint8_t>& buffer)
    {
        while (value >= 0x80)
        {
            buffer.push_back(static_cast<std::uint8_t>(value | 0x80));
            value >>= 7;
        }
        buffer.push_back(static_cast<std::uint8_t>(value));
    }

    // LEB128 encoder for uint64 test values
    static void EncodeLEB128_64(std::uint64_t value, std::vector<std::uint8_t>& buffer)
    {
        while (value >= 0x80)
        {
            buffer.push_back(static_cast<std::uint8_t>(value | 0x80));
            value >>= 7;
        }
        buffer.push_back(static_cast<std::uint8_t>(value));
    }

    // Writes a fixed-width little-endian uint64 to the buffer
    static void EncodeFixedUInt64(std::uint64_t value, std::vector<std::uint8_t>& buffer)
    {
        std::uint8_t bytes[8];
        std::memcpy(bytes, &value, 8);
        buffer.insert(buffer.end(), bytes, bytes + 8);
    }

    // Creates a XamlBinaryFormatSubReader2 with the given buffer as its node stream.
    // Uses a null master reader — safe for ReadUInt/ReadUInt64/ReadFixedWidthUInt64
    // which only access the node stream buffer fields.
    static XamlBinaryFormatSubReader2 CreateTestReader(std::vector<std::uint8_t>& buffer)
    {
        return XamlBinaryFormatSubReader2(
            nullptr,    // master reader (unused by primitive read methods)
            0,          // nodeStreamMasterOffset
            static_cast<unsigned int>(buffer.size()),
            0,          // lineStreamLength
            buffer.data(),
            nullptr);   // lineStream
    }

    #pragma region Test Class Initialization & Cleanup
    bool XbfSubReaderUnitTests::ClassSetup()
    {
        return true;
    }

    bool XbfSubReaderUnitTests::ClassCleanup()
    {
        return true;
    }
    #pragma endregion

    // Helper to verify a single ReadUInt value from a fresh reader
    static void VerifyReadUInt(std::uint32_t expected)
    {
        std::vector<std::uint8_t> buffer;
        EncodeLEB128(expected, buffer);
        // Pad to at least 8 bytes so the fast path can engage on x64/ARM64
        while (buffer.size() < 8)
        {
            buffer.push_back(0x00);
        }
        auto reader = CreateTestReader(buffer);
        unsigned int actual = reader.ReadUInt();
        VERIFY_ARE_EQUAL(expected, actual);
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_SingleByteValues()
    {
        VerifyReadUInt(0);
        VerifyReadUInt(1);
        VerifyReadUInt(42);
        VerifyReadUInt(127);
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_TwoByteValues()
    {
        VerifyReadUInt(128);
        VerifyReadUInt(300);
        VerifyReadUInt(16383);
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_ThreeToFiveBytes()
    {
        VerifyReadUInt(16384);          // 3 bytes
        VerifyReadUInt(2097152);        // 4 bytes
        VerifyReadUInt(0x80000000);     // 5 bytes, tests bit 31 extraction
        VerifyReadUInt(0xFFFFFFFF);     // 5 bytes, UINT32_MAX
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_SequentialValues()
    {
        // Encode two varints back-to-back: 300 (2 bytes) then 150 (2 bytes)
        std::vector<std::uint8_t> buffer;
        EncodeLEB128(300, buffer);
        EncodeLEB128(150, buffer);
        // Pad to at least 8 bytes for fast-path engagement
        while (buffer.size() < 8)
        {
            buffer.push_back(0x00);
        }

        auto reader = CreateTestReader(buffer);
        VERIFY_ARE_EQUAL(static_cast<unsigned int>(300), reader.ReadUInt());
        VERIFY_ARE_EQUAL(static_cast<unsigned int>(150), reader.ReadUInt());
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_SlowPathFallback()
    {
        // Create a buffer containing only the varint bytes with no padding, so
        // remaining bytes < speculative load size (8 on x64/ARM64, 5 on x86),
        // forcing the slow-path fallback.
        std::uint32_t expected = 300; // 2-byte varint
        std::vector<std::uint8_t> buffer;
        EncodeLEB128(expected, buffer);

        auto reader = CreateTestReader(buffer);
        VERIFY_ARE_EQUAL(expected, reader.ReadUInt());
    }

    void XbfSubReaderUnitTests::Verify7BitDecode_MalformedOverlong()
    {
        // 8 bytes of continuation (all 0x80) — overlong encoding, should fail.
        // On x64/ARM64 fast path: 5 continuation bytes with no terminator → return false → throw.
        std::vector<std::uint8_t> buffer = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
        auto reader = CreateTestReader(buffer);

        bool threw = false;
        try
        {
            reader.ReadUInt();
        }
        catch (...)
        {
            threw = true;
        }
        VERIFY_IS_TRUE(threw);
    }

    void XbfSubReaderUnitTests::Verify7BitDecode64_SmallAndLarge()
    {
        // Small value: 42 (1 byte)
        {
            std::vector<std::uint8_t> buffer;
            EncodeLEB128_64(42, buffer);
            while (buffer.size() < 16)
            {
                buffer.push_back(0x00);
            }
            auto reader = CreateTestReader(buffer);
            VERIFY_ARE_EQUAL(static_cast<std::uint64_t>(42), reader.ReadUInt64());
        }

        // Max value: UINT64_MAX (10 bytes in LEB128)
        {
            std::vector<std::uint8_t> buffer;
            EncodeLEB128_64(std::numeric_limits<std::uint64_t>::max(), buffer);
            while (buffer.size() < 16)
            {
                buffer.push_back(0x00);
            }
            auto reader = CreateTestReader(buffer);
            VERIFY_ARE_EQUAL(std::numeric_limits<std::uint64_t>::max(), reader.ReadUInt64());
        }
    }

    void XbfSubReaderUnitTests::VerifyFixedWidthUInt64_Values()
    {
        // Test zero
        {
            std::vector<std::uint8_t> buffer;
            EncodeFixedUInt64(0, buffer);
            auto reader = CreateTestReader(buffer);
            VERIFY_ARE_EQUAL(static_cast<std::uint64_t>(0), reader.ReadFixedWidthUInt64());
        }

        // Test UINT64_MAX
        {
            std::vector<std::uint8_t> buffer;
            EncodeFixedUInt64(std::numeric_limits<std::uint64_t>::max(), buffer);
            auto reader = CreateTestReader(buffer);
            VERIFY_ARE_EQUAL(std::numeric_limits<std::uint64_t>::max(), reader.ReadFixedWidthUInt64());
        }

        // Test hash-like value
        {
            const std::uint64_t hashValue = 0xDEADBEEFCAFEF00DULL;
            std::vector<std::uint8_t> buffer;
            EncodeFixedUInt64(hashValue, buffer);
            auto reader = CreateTestReader(buffer);
            VERIFY_ARE_EQUAL(hashValue, reader.ReadFixedWidthUInt64());
        }
    }

    void XbfSubReaderUnitTests::VerifyFixedWidthUInt64_Truncated()
    {
        // Buffer with only 7 bytes — not enough for an 8-byte read
        std::vector<std::uint8_t> buffer = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
        auto reader = CreateTestReader(buffer);

        bool threw = false;
        try
        {
            reader.ReadFixedWidthUInt64();
        }
        catch (...)
        {
            threw = true;
        }
        VERIFY_IS_TRUE(threw);
    }

    void XbfSubReaderUnitTests::VerifyFixedWidthUInt64_UnalignedOffset()
    {
        // 9-byte buffer, set offset to 1 so the 8-byte read starts at offset 1
        const std::uint64_t expected = 0x0102030405060708ULL;
        std::vector<std::uint8_t> buffer;
        buffer.push_back(0xFF); // dummy byte at offset 0
        EncodeFixedUInt64(expected, buffer);

        auto reader = CreateTestReader(buffer);
        reader.set_NextIndex(1); // advance past dummy byte
        VERIFY_ARE_EQUAL(expected, reader.ReadFixedWidthUInt64());
    }

    void XbfSubReaderUnitTests::VerifyReadUInt_EmptyBuffer()
    {
        std::vector<std::uint8_t> buffer; // empty
        // CreateTestReader with empty buffer data pointer is buffer.data() which may be null
        // Use a 1-byte buffer but set length to 0 to avoid null pointer issues
        std::uint8_t dummy = 0;
        XamlBinaryFormatSubReader2 reader(nullptr, 0, 0, 0, &dummy, nullptr);

        bool threw = false;
        try
        {
            reader.ReadUInt();
        }
        catch (...)
        {
            threw = true;
        }
        VERIFY_IS_TRUE(threw);
    }

    void XbfSubReaderUnitTests::VerifyReadUInt_TruncatedVarint()
    {
        // Single byte with continuation bit set — expects more bytes but buffer ends
        std::vector<std::uint8_t> buffer = { 0x80 };
        auto reader = CreateTestReader(buffer);

        bool threw = false;
        try
        {
            reader.ReadUInt();
        }
        catch (...)
        {
            threw = true;
        }
        VERIFY_IS_TRUE(threw);
    }

} } } } }
