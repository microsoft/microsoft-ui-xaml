// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace WindowPlacement {

    class WindowPlacementUnitTests
    {
    public:
        BEGIN_TEST_CLASS(WindowPlacementUnitTests)
        END_TEST_CLASS()

        // Serializer / parser round trips.
        BEGIN_TEST_METHOD(RoundTripMinimal)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"A normal-rect-only placement survives a serialize/parse round trip")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RoundTripAllFields)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"A placement with every v1 field survives a serialize/parse round trip")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(Base64RoundTrip)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"A placement survives a Base64 serialize/parse round trip")
        END_TEST_METHOD()

        // Golden format stability.
        BEGIN_TEST_METHOD(GoldenBlobV1)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"The v1 writer produces the exact checked-in canonical bytes")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(WriterIsCanonicalRegardlessOfSetOrder)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"Tags are emitted in ascending order regardless of the order fields were populated")
        END_TEST_METHOD()

        // Fail-safe / compatibility rules.
        BEGIN_TEST_METHOD(SerializeEmptyWithoutNormalRect)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"Serializing a placement without a normal rect yields nothing")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectMissingNormalRect)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"A blob whose only field is not a normal rect is not a usable placement")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectBadMagic)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectUnknownMajor)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ToleratesUnknownMinor)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectWrongTotalLength)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectTruncatedField)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectWrongFixedWidthLength)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectOutOfRangeCoordinate)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectOddDeviceNameLength)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectOversizeDeviceName)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RejectOversizeBlob)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SkipUnknownTag)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"An unknown tag is skipped by its length and does not invalidate the blob")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnknownShowCmdTreatedAsAbsent)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(UnknownFlagBitsIgnored)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DuplicateTagLastWins)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(InvalidBase64ReturnsFalse)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()

        // Value-name derivation.
        BEGIN_TEST_METHOD(ValueNameShape)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"The value name is wp1_<slug>_<52-char base32 hash>")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SlugRules)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"Description", L"The slug keeps up to 16 ASCII alphanumerics, else uses 'id'")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(HashIsDeterministicAndDistinct)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
        END_TEST_METHOD()
    };

} } } } }
