// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace WindowPlacement {

class WindowPlacementBlobUnitTests : public WEX::TestClass<WindowPlacementBlobUnitTests>
{
public:
    BEGIN_TEST_CLASS(WindowPlacementBlobUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Unit")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(GoldenMinimalBlob)
        TEST_METHOD_PROPERTY(L"Description", L"A minimal placement serializes to the exact expected canonical bytes.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(GoldenFullBlob)
        TEST_METHOD_PROPERTY(L"Description", L"A placement using every optional tag serializes to the exact expected canonical bytes.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundTripsMinimalPlacement)
        TEST_METHOD_PROPERTY(L"Description", L"Serialize then deserialize preserves the required fields.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundTripsFullPlacement)
        TEST_METHOD_PROPERTY(L"Description", L"Serialize then deserialize preserves every optional field.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RoundTripsThroughBase64)
        TEST_METHOD_PROPERTY(L"Description", L"TryEncodePlacement and TryDecodePlacement round trip a placement.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsBadHeader)
        TEST_METHOD_PROPERTY(L"Description", L"Wrong magic, wrong major version, and short buffers are rejected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsWrongTotalLength)
        TEST_METHOD_PROPERTY(L"Description", L"A total length that disagrees with the buffer size is rejected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsTruncatedBlob)
        TEST_METHOD_PROPERTY(L"Description", L"Truncating a valid blob at every offset never succeeds and never reads out of bounds.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsOversizedBlob)
        TEST_METHOD_PROPERTY(L"Description", L"A blob larger than the 4 KiB cap is rejected before allocation.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsMissingRequiredTags)
        TEST_METHOD_PROPERTY(L"Description", L"Dropping normal rect, work area, or DPI rejects the blob.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsWrongFixedTagLength)
        TEST_METHOD_PROPERTY(L"Description", L"A known fixed-width tag with the wrong length rejects the whole blob.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsInvalidRects)
        TEST_METHOD_PROPERTY(L"Description", L"Empty, inverted, and out-of-range rectangles are rejected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsInvalidDpi)
        TEST_METHOD_PROPERTY(L"Description", L"DPI below 96 or above the supported maximum is rejected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsInvalidDeviceName)
        TEST_METHOD_PROPERTY(L"Description", L"Odd length, oversized, and embedded-NUL device names are rejected.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SkipsUnknownTags)
        TEST_METHOD_PROPERTY(L"Description", L"Unknown tags are skipped using their encoded length.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(AcceptsHigherMinorVersion)
        TEST_METHOD_PROPERTY(L"Description", L"A higher minor version with major version 1 is accepted.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DuplicateTagsLastOneWins)
        TEST_METHOD_PROPERTY(L"Description", L"A repeated tag uses the last occurrence.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TreatsZeroLengthDeviceNameAsAbsent)
        TEST_METHOD_PROPERTY(L"Description", L"A zero-length device name decodes to an empty name rather than failing.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TreatsNullVirtualDesktopIdAsAbsent)
        TEST_METHOD_PROPERTY(L"Description", L"GUID_NULL decodes as no virtual desktop id.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanonicalizesShowCommand)
        TEST_METHOD_PROPERTY(L"Description", L"Normal, maximized, and minimized aliases canonicalize; hidden and unknown default to normal.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DropsUnknownFlagBits)
        TEST_METHOD_PROPERTY(L"Description", L"Unknown flag bits are ignored and never surfaced to the caller.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DropsArrangedFlagsWithoutArrangeRect)
        TEST_METHOD_PROPERTY(L"Description", L"Arranged and RestoreToArranged are dropped when no valid arrange rect is present.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(WriterRejectsUnwritablePlacement)
        TEST_METHOD_PROPERTY(L"Description", L"The writer refuses placements its own reader would reject.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsGarbageBase64)
        TEST_METHOD_PROPERTY(L"Description", L"Non-base64 text and base64 of non-placement bytes are rejected.")
    END_TEST_METHOD()
};

class WindowPlacementValueNameUnitTests : public WEX::TestClass<WindowPlacementValueNameUnitTests>
{
public:
    BEGIN_TEST_CLASS(WindowPlacementValueNameUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Unit")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(GoldenValueName)
        TEST_METHOD_PROPERTY(L"Description", L"The worked example from the design produces the expected value name.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SlugKeepsOnlyAsciiAlphanumerics)
        TEST_METHOD_PROPERTY(L"Description", L"Punctuation, spaces, and non-ASCII characters are dropped from the slug.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SlugStopsAfterSixteenCharacters)
        TEST_METHOD_PROPERTY(L"Description", L"The slug keeps at most 16 accepted characters.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SlugFallsBackWhenNothingAccepted)
        TEST_METHOD_PROPERTY(L"Description", L"An id with no ASCII alphanumerics produces the 'id' slug.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Sha256MatchesKnownVector)
        TEST_METHOD_PROPERTY(L"Description", L"SHA-256 over UTF-16LE code units matches an independently computed digest.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(Base32MatchesRfc4648Vectors)
        TEST_METHOD_PROPERTY(L"Description", L"Base32 encoding matches the RFC 4648 test vectors with padding removed.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ValueNameIsStable)
        TEST_METHOD_PROPERTY(L"Description", L"The same id always produces the same value name.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DistinctIdsWithSameSlugDiffer)
        TEST_METHOD_PROPERTY(L"Description", L"Ids that share a slug still produce different value names.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(IdsDifferingOnlyByCaseDiffer)
        TEST_METHOD_PROPERTY(L"Description", L"Case-insensitive value-name comparison still distinguishes case-different ids.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RejectsEmptyId)
        TEST_METHOD_PROPERTY(L"Description", L"An empty or null id does not produce a value name.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HandlesLongAndUnicodeIds)
        TEST_METHOD_PROPERTY(L"Description", L"Long ids and ids with surrogate pairs produce well-formed value names.")
    END_TEST_METHOD()
};

} } } } }
