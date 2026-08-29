// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minerror.h>
#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XbfSubReaderUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XbfSubReaderUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        // 7-bit encoded uint32 (fast path + slow path)
        BEGIN_TEST_METHOD(Verify7BitDecode_SingleByteValues)
            TEST_METHOD_PROPERTY(L"Description", L"Decodes 1-byte LEB128 values: 0, 1, 42, 127")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Verify7BitDecode_TwoByteValues)
            TEST_METHOD_PROPERTY(L"Description", L"Decodes 2-byte LEB128 values: 128, 300, 16383")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Verify7BitDecode_ThreeToFiveBytes)
            TEST_METHOD_PROPERTY(L"Description", L"Decodes 3-5 byte LEB128 values: 16384, 2097152, 0x80000000, UINT32_MAX")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Verify7BitDecode_SequentialValues)
            TEST_METHOD_PROPERTY(L"Description", L"Decodes two back-to-back LEB128 uint32 values from a single buffer")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Verify7BitDecode_SlowPathFallback)
            TEST_METHOD_PROPERTY(L"Description", L"Forces slow-path fallback by placing varint near end of buffer")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Verify7BitDecode_MalformedOverlong)
            TEST_METHOD_PROPERTY(L"Description", L"Rejects overlong encoding with >5 continuation bytes")
        END_TEST_METHOD()

        // 7-bit encoded uint64 (slow path)
        BEGIN_TEST_METHOD(Verify7BitDecode64_SmallAndLarge)
            TEST_METHOD_PROPERTY(L"Description", L"Decodes uint64 LEB128 values: 42 (1 byte) and UINT64_MAX (10 bytes)")
        END_TEST_METHOD()

        // Fixed-width uint64
        BEGIN_TEST_METHOD(VerifyFixedWidthUInt64_Values)
            TEST_METHOD_PROPERTY(L"Description", L"Reads fixed-width uint64 values: 0, UINT64_MAX, hash-like value")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFixedWidthUInt64_Truncated)
            TEST_METHOD_PROPERTY(L"Description", L"Rejects fixed-width uint64 read when buffer has fewer than 8 bytes")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFixedWidthUInt64_UnalignedOffset)
            TEST_METHOD_PROPERTY(L"Description", L"Reads fixed-width uint64 at non-zero buffer offset")
        END_TEST_METHOD()

        // Error cases
        BEGIN_TEST_METHOD(VerifyReadUInt_EmptyBuffer)
            TEST_METHOD_PROPERTY(L"Description", L"ReadUInt on empty buffer throws E_FAIL")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyReadUInt_TruncatedVarint)
            TEST_METHOD_PROPERTY(L"Description", L"ReadUInt with truncated varint (continuation bit set, no next byte) throws")
        END_TEST_METHOD()
    };

} } } } }
