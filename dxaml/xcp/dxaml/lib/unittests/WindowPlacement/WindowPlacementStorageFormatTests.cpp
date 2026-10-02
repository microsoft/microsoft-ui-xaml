// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementStorageFormat.h"

using namespace DirectUI::WindowPlacementPersistence;

namespace
{
    // Independent base64 of the 76-byte golden record: normal (-20,-10,40,30),
    // work (-100,-50,200,100), 96 DPI, normal state.
    const std::u16string GoldenRecordText =
        u"V1BMMQEAAABMAAAAAQAQAOz////2////FAAAABQAAAACABAAnP///87///9kAAAAMgAAABAABABgAAAAEQAEAAEAAAASAAQAAAAAAA==";

    // Independent Base32(SHA-256(UTF-16LE(id))) digests, uppercase RFC 4648 without padding.
    const std::u16string MainWindowDigest = u"WB75XK3P3EI6TOSVS4IG3WMQ6VTA54RUHOLRSYYZP5VUTLYGKZHA";
    const std::u16string LowerMainWindowDigest = u"LU336YUZIASKGPV64ALYLLUCLWPUPALGWSTMXJOLETSEOBNPX3MA";
    const std::u16string DocumentWindowDigest = u"5Z6JCUH7DMIYL5L4DSZYMQN7DVDP53MAQZQVFZ4XNQHY5QAFPTTA";
    const std::u16string CjkDigest = u"EEPQM6BVU2TLG7PC52ACUJC7WOIZBKOTP7JT4UGHEO3XW5CPKAZQ";
    const std::u16string SurrogateDigest = u"6FS7HG5QVKU3RPHNP2VIV5LWQIGKQVTD2MJIJSNHX3C7ETT7D6SQ";

    Record Sample()
    {
        Record record;
        record.Placement.NormalRect = {-20, -10, 40, 30};
        record.Placement.WorkArea = {-100, -50, 200, 100};
        record.Placement.Dpi = 96;
        return record;
    }

    std::u16string Encode(const Record& record)
    {
        std::u16string text;
        VERIFY_IS_TRUE(EncodeRecordText(record, text));
        return text;
    }

    DecodeResult Decode(const std::u16string& text, Record& record)
    {
        return DecodeRecordText(text.c_str(), text.size(), record);
    }

    void RejectText(const std::u16string& text)
    {
        auto output = Sample();
        output.SaveSequence = 42;
        VERIFY_IS_TRUE(Decode(text, output) == DecodeResult::InvalidData);
        VERIFY_ARE_EQUAL(uint64_t{42}, output.SaveSequence);
        VERIFY_ARE_EQUAL(96, output.Placement.Dpi);
        VERIFY_ARE_EQUAL(-20, output.Placement.NormalRect.X);
    }

    size_t PaddingCount(const std::u16string& text)
    {
        size_t count = 0;
        while (count < text.size() && text[text.size() - 1 - count] == u'=') ++count;
        return count;
    }

    // Replaces the final data character with one whose low bits are not canonically zero.
    std::u16string CorruptTrailingBits(const std::u16string& text)
    {
        auto corrupted = text;
        corrupted[text.size() - PaddingCount(text) - 1] = u'B';
        return corrupted;
    }

    std::u16string MakeValueName(const std::u16string& id)
    {
        std::u16string name;
        VERIFY_IS_TRUE(TryMakePlacementValueName(id.c_str(), id.size(), name));
        return name;
    }

    std::u16string MakeContainerName(const std::u16string& id)
    {
        std::u16string name;
        VERIFY_IS_TRUE(TryMakeApplicationContainerName(id.c_str(), id.size(), name));
        return name;
    }

    std::u16string MakeGroupPropertyName(
        const std::u16string& applicationId,
        const std::u16string& placementId)
    {
        std::u16string name;
        VERIFY_IS_TRUE(TryMakePlacementGroupPropertyName(
            applicationId.c_str(), applicationId.size(),
            placementId.c_str(), placementId.size(), name));
        return name;
    }
}

// Base64 transport for the binary record. No storage, window, or display state is involved.
class WindowPlacementRecordTextTests
{
public:
    TEST_CLASS(WindowPlacementRecordTextTests);

    TEST_METHOD(GoldenText)
    {
        VERIFY_IS_TRUE(Encode(Sample()) == GoldenRecordText);

        Record decoded;
        VERIFY_IS_TRUE(Decode(GoldenRecordText, decoded) == DecodeResult::Success);
        VERIFY_ARE_EQUAL(-20, decoded.Placement.NormalRect.X);
        VERIFY_ARE_EQUAL(-10, decoded.Placement.NormalRect.Y);
        VERIFY_ARE_EQUAL(40, decoded.Placement.NormalRect.Width);
        VERIFY_ARE_EQUAL(30, decoded.Placement.NormalRect.Height);
        VERIFY_ARE_EQUAL(200, decoded.Placement.WorkArea.Width);
        VERIFY_ARE_EQUAL(96, decoded.Placement.Dpi);
        VERIFY_IS_TRUE(decoded.Placement.PlacementState == State::Normal);
        VERIFY_ARE_EQUAL(uint64_t{0}, decoded.SaveSequence);
    }

    // Records of every length remainder must round trip, including both padded forms.
    TEST_METHOD(RoundTripsEveryPaddingLength)
    {
        auto twoPadding = Sample();
        auto onePadding = Sample();
        onePadding.Placement.DisplayDeviceName = u"ABC";
        auto noPadding = Sample();
        noPadding.Placement.DisplayDeviceName = u"AB";

        VERIFY_ARE_EQUAL(size_t{2}, PaddingCount(Encode(twoPadding)));
        VERIFY_ARE_EQUAL(size_t{1}, PaddingCount(Encode(onePadding)));
        VERIFY_ARE_EQUAL(size_t{0}, PaddingCount(Encode(noPadding)));

        for (const auto& record : {twoPadding, onePadding, noPadding})
        {
            const auto text = Encode(record);
            VERIFY_ARE_EQUAL(size_t{0}, text.size() % 4);

            Record decoded;
            VERIFY_IS_TRUE(Decode(text, decoded) == DecodeResult::Success);
            VERIFY_IS_TRUE(decoded.Placement.DisplayDeviceName == record.Placement.DisplayDeviceName);
            VERIFY_ARE_EQUAL(40, decoded.Placement.NormalRect.Width);
            VERIFY_IS_TRUE(Encode(decoded) == text);
        }
    }

    TEST_METHOD(RoundTripsOptionalMetadata)
    {
        auto record = Sample();
        record.Placement.PlacementState = State::MinimizedFromSnapped;
        record.Placement.SnapRect = Rect{0, 0, 10, 20};
        record.Placement.DisplayDeviceName = u"\\\\.\\DISPLAY1";
        record.Placement.VirtualDesktopId = GUID{0x11223344, 0x5566, 0x7788, {1, 2, 3, 4, 5, 6, 7, 8}};
        record.SaveSequence = 0x0102030405060708ull;

        Record decoded;
        VERIFY_IS_TRUE(Decode(Encode(record), decoded) == DecodeResult::Success);
        VERIFY_IS_TRUE(decoded.Placement.PlacementState == State::MinimizedFromSnapped);
        VERIFY_IS_TRUE(decoded.Placement.SnapRect.has_value());
        VERIFY_ARE_EQUAL(10, decoded.Placement.SnapRect->Width);
        VERIFY_IS_TRUE(decoded.Placement.DisplayDeviceName == u"\\\\.\\DISPLAY1");
        VERIFY_IS_TRUE(decoded.Placement.VirtualDesktopId.has_value());
        VERIFY_IS_TRUE(decoded.Placement.VirtualDesktopId->Data1 == 0x11223344);
        VERIFY_ARE_EQUAL(uint64_t{0x0102030405060708ull}, decoded.SaveSequence);
    }

    TEST_METHOD(RejectsMalformedText)
    {
        Record record;
        VERIFY_IS_TRUE(DecodeRecordText(nullptr, 4, record) == DecodeResult::InvalidData);
        VERIFY_IS_TRUE(DecodeRecordText(GoldenRecordText.c_str(), 0, record) == DecodeResult::InvalidData);

        RejectText(GoldenRecordText.substr(0, GoldenRecordText.size() - 1)); // Not a multiple of four.
        RejectText(GoldenRecordText.substr(0, GoldenRecordText.size() - 4)); // Truncated record.

        auto invalidCharacter = GoldenRecordText;
        invalidCharacter[0] = u'-';
        RejectText(invalidCharacter);

        auto misplacedPadding = GoldenRecordText;
        misplacedPadding[4] = u'=';
        RejectText(misplacedPadding);

        auto extraPadding = GoldenRecordText;
        extraPadding[extraPadding.size() - 3] = u'=';
        RejectText(extraPadding); // Three padding characters are never canonical.
    }

    // One record must have exactly one encoding, so unused tail bits must be zero.
    TEST_METHOD(RejectsNonCanonicalTrailingBits)
    {
        auto onePadding = Sample();
        onePadding.Placement.DisplayDeviceName = u"ABC";

        RejectText(CorruptTrailingBits(GoldenRecordText));
        RejectText(CorruptTrailingBits(Encode(onePadding)));
    }

    // The encoded bound is checked before any decode buffer is allocated.
    TEST_METHOD(RejectsOversizedText)
    {
        // The bound must stay tied to the decoded ceiling rather than a hand-copied number.
        VERIFY_ARE_EQUAL(size_t{5464}, MaximumEncodedCharacters);
        VERIFY_IS_TRUE(MaximumEncodedCharacters / 4 * 3 >= MaximumRecordBytes);

        Record record;
        const std::u16string overLimit(MaximumEncodedCharacters + 4, u'A');
        VERIFY_IS_TRUE(Decode(overLimit, record) == DecodeResult::InvalidData);

        // At the limit the text is parsed, but the decoded record exceeds MaximumRecordBytes.
        const std::u16string atLimit(MaximumEncodedCharacters, u'A');
        VERIFY_IS_TRUE(Decode(atLimit, record) == DecodeResult::InvalidData);
    }

    // An unencodable record must not leave a partial or stale value in the output string.
    TEST_METHOD(LeavesTextUnchangedWhenEncodingFails)
    {
        auto invalid = Sample();
        invalid.Placement.NormalRect.Width = 0;

        std::u16string text = u"previous";
        VERIFY_IS_FALSE(EncodeRecordText(invalid, text));
        VERIFY_IS_TRUE(text == u"previous");
    }

    TEST_METHOD(ReportsUnsupportedVersion)
    {
        auto text = GoldenRecordText;
        text[6] = u'I'; // Major version 2 in the encoded header.

        Record record;
        VERIFY_IS_TRUE(Decode(text, record) == DecodeResult::UnsupportedVersion);
    }
};

// Private container and value names derived from the application and placement ids.
class WindowPlacementStorageNameTests
{
public:
    TEST_CLASS(WindowPlacementStorageNameTests);

    TEST_METHOD(GoldenNames)
    {
        VERIFY_IS_TRUE(MakeContainerName(u"MainWindow") == u"app1_" + MainWindowDigest);
        VERIFY_IS_TRUE(MakeValueName(u"MainWindow") == u"wp1_MainWindow_" + MainWindowDigest);
    }

    // Ids keep ordinal, case-sensitive semantics even though the backend may not.
    TEST_METHOD(DistinguishesCase)
    {
        VERIFY_IS_TRUE(MakeContainerName(u"mainwindow") == u"app1_" + LowerMainWindowDigest);
        VERIFY_IS_TRUE(MakeValueName(u"mainwindow") == u"wp1_mainwindow_" + LowerMainWindowDigest);
        VERIFY_IS_FALSE(MakeValueName(u"MainWindow") == MakeValueName(u"mainwindow"));
    }

    TEST_METHOD(SlugKeepsLeadingAsciiAlphanumerics)
    {
        VERIFY_IS_TRUE(MakeValueName(u"DocumentWindow:doc42") ==
            u"wp1_DocumentWindowdo_" + DocumentWindowDigest);
    }

    TEST_METHOD(SlugFallsBackWhenNothingIsAccepted)
    {
        VERIFY_IS_TRUE(MakeValueName(u"\u4e2d\u6587 :/") == u"wp1_id_" + CjkDigest);
    }

    // Ids are hashed as UTF-16 code units, so surrogate pairs and composed/decomposed
    // sequences must not be reordered, normalized, or collapsed.
    TEST_METHOD(PreservesSurrogatesAndComposition)
    {
        VERIFY_IS_TRUE(MakeValueName(u"win\U0001F600 42") == u"wp1_win42_" + SurrogateDigest);
        VERIFY_IS_FALSE(MakeValueName(u"caf\u00e9") == MakeValueName(u"cafe\u0301"));
    }

    TEST_METHOD(NamesStayWithinDocumentedLengths)
    {
        const std::u16string longId(40, u'a');
        VERIFY_ARE_EQUAL(size_t{57}, MakeContainerName(longId).size());

        const auto valueName = MakeValueName(longId);
        VERIFY_ARE_EQUAL(size_t{73}, valueName.size());
        VERIFY_IS_TRUE(valueName.compare(0, 21, u"wp1_aaaaaaaaaaaaaaaa_") == 0);
    }

    // Hashing uses the supplied length, not a NUL-terminated string function.
    TEST_METHOD(HashesEveryCodeUnit)
    {
        const std::u16string withEmbeddedNull{u'a', u'\0', u'b'};
        const auto name = MakeValueName(withEmbeddedNull);
        VERIFY_IS_TRUE(name.compare(0, 7, u"wp1_ab_") == 0);
        VERIFY_IS_FALSE(name == MakeValueName(u"ab"));
        VERIFY_IS_FALSE(name == MakeValueName(u"a"));
    }

    TEST_METHOD(RejectsEmptyIds)
    {
        std::u16string name = u"unchanged";
        VERIFY_IS_FALSE(TryMakeApplicationContainerName(u"", 0, name));
        VERIFY_IS_FALSE(TryMakePlacementValueName(u"", 0, name));
        VERIFY_IS_FALSE(TryMakeApplicationContainerName(nullptr, 4, name));
        VERIFY_IS_FALSE(TryMakePlacementValueName(nullptr, 4, name));
        VERIFY_IS_TRUE(name == u"unchanged");
    }

    // The two identity components stay separate: neither name is built from a compound key.
    TEST_METHOD(SeparatesApplicationAndPlacementIdentity)
    {
        VERIFY_IS_TRUE(MakeContainerName(u"MainWindow") == MakeContainerName(u"MainWindow"));
        VERIFY_IS_FALSE(MakeContainerName(u"App.A") == MakeContainerName(u"App.B"));
        VERIFY_IS_TRUE(MakeValueName(u"MainWindow").compare(4, 11, u"MainWindow_") == 0);
    }

    TEST_METHOD(GroupMarkersUseBothExactIdentityComponents)
    {
        const auto packaged = MakeGroupPropertyName(u"Contoso.App_123", u"MainWindow");
        VERIFY_IS_TRUE(packaged.compare(0, 36, u"Microsoft.UI.Xaml.PlacementGroup.v1.") == 0);
        VERIFY_IS_TRUE(packaged == MakeGroupPropertyName(u"Contoso.App_123", u"MainWindow"));
        VERIFY_IS_FALSE(packaged == MakeGroupPropertyName(u"Contoso.App_124", u"MainWindow"));
        VERIFY_IS_FALSE(packaged == MakeGroupPropertyName(u"Contoso.App_123", u"mainwindow"));

        const std::u16string embeddedApplication{u'a', u'\0', u'b'};
        const std::u16string embeddedPlacement{u'b', u'\0', u'c'};
        VERIFY_IS_FALSE(
            MakeGroupPropertyName(embeddedApplication, u"c") ==
            MakeGroupPropertyName(u"a", embeddedPlacement));
    }

    TEST_METHOD(GroupMarkersRejectInvalidIds)
    {
        std::u16string name = u"unchanged";
        VERIFY_IS_FALSE(TryMakePlacementGroupPropertyName(
            nullptr, 1, u"MainWindow", 10, name));
        VERIFY_IS_FALSE(TryMakePlacementGroupPropertyName(
            u"App", 3, u"", 0, name));
        VERIFY_IS_TRUE(name == u"unchanged");
    }
};
