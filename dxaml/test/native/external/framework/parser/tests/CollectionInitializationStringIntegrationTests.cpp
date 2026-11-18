// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <StringUtilities.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <cmath>
#include <string>
#include <vector>
#include "CollectionInitializationStringIntegrationTests.h"

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    namespace
    {
        const HRESULT c_xamlParseFailed = static_cast<HRESULT>(0x802B000A);

        // Test infrastructure enables EnforceXbfV2Stream for every test (text XAML is converted to an XBFv2
        // stream before loading), while the product default is disabled (text is written directly by the
        // ObjectWriter). Run each scenario on both routes.
        template <typename TBody>
        void RunOnBothParserRoutes(TBody body)
        {
            for (bool enforceXbfV2Stream : { true, false })
            {
                RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, enforceXbfV2Stream);
                LOG_OUTPUT(L"Parser route: EnforceXbfV2Stream=%s", enforceXbfV2Stream ? L"true" : L"false");
                RunOnUIThread(body);
            }
        }
        // TransformGroup.Children is a collection property whose type (TransformCollection) has no type
        // converter while its item type (Transform) converts from a matrix string ("m11 m12 m21 m22 dx dy"),
        // so a string attribute value is split by the collection initialization-string parser and each
        // entry becomes a MatrixTransform.
        String^ MakeTransformGroupMarkup(const wchar_t* childrenAttributeValue)
        {
            std::wstring markup(L"<TransformGroup xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Children=\"");
            markup += childrenAttributeValue;
            markup += L"\" />";
            return ref new String(markup.c_str());
        }

        struct ExpectedOffset
        {
            double x;
            double y;
        };

        bool AreClose(double expected, double actual)
        {
            return std::fabs(expected - actual) < 1e-6;
        }

        // Every entry used by these tests is a translation matrix "1 0 0 1 dx dy" (or Identity), so the
        // per-item offsets identify which entry produced which child and prove the order.
        void VerifyChildren(const wchar_t* childrenAttributeValue, const std::vector<ExpectedOffset>& expected)
        {
            LOG_OUTPUT(L"Loading Children=\"%s\"", childrenAttributeValue);
            auto group = safe_cast<TransformGroup^>(XamlReader::Load(MakeTransformGroupMarkup(childrenAttributeValue)));

            VERIFY_ARE_EQUAL(static_cast<unsigned int>(expected.size()), group->Children->Size);
            for (unsigned int i = 0; i < expected.size(); i++)
            {
                auto transform = dynamic_cast<MatrixTransform^>(group->Children->GetAt(i));
                VERIFY_IS_NOT_NULL(transform, L"Each initialization-string entry is converted to a MatrixTransform.");
                auto matrix = transform->Matrix;
                LOG_OUTPUT(L"  child %u: M11=%f M22=%f OffsetX=%f OffsetY=%f", i, matrix.M11, matrix.M22, matrix.OffsetX, matrix.OffsetY);
                VERIFY_IS_TRUE(AreClose(1.0, matrix.M11) && AreClose(0.0, matrix.M12) && AreClose(0.0, matrix.M21) && AreClose(1.0, matrix.M22),
                    L"Entries are pure translations.");
                VERIFY_IS_TRUE(AreClose(expected[i].x, matrix.OffsetX), L"Unexpected OffsetX (wrong entry or order).");
                VERIFY_IS_TRUE(AreClose(expected[i].y, matrix.OffsetY), L"Unexpected OffsetY (wrong entry or order).");
            }
        }

        void VerifyLoadFails(const wchar_t* childrenAttributeValue, const wchar_t* expectedMessageFragment)
        {
            LOG_OUTPUT(L"Loading Children=\"%s\" (expected to fail)", childrenAttributeValue);
            bool threw = false;
            HRESULT hr = S_OK;
            std::wstring message;
            try
            {
                XamlReader::Load(MakeTransformGroupMarkup(childrenAttributeValue));
            }
            catch (Exception^ e)
            {
                threw = true;
                hr = static_cast<HRESULT>(e->HResult);
                if (e->Message != nullptr)
                {
                    message = e->Message->Data();
                }
            }

            VERIFY_IS_TRUE(threw, L"Loading malformed collection initialization markup must fail.");
            VERIFY_ARE_EQUAL(c_xamlParseFailed, hr);
            LOG_OUTPUT(L"Error message: %s", message.c_str());
            VERIFY_IS_TRUE(message.find(expectedMessageFragment) != std::wstring::npos, expectedMessageFragment);
        }
    }

    bool CollectionInitializationStringIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CollectionInitializationStringIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CollectionInitializationStringIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringCreatesItems()
    {
        TestCleanupWrapper cleanup;

        RunOnBothParserRoutes([&]()
        {
            VerifyChildren(L"1 0 0 1 5 0,1 0 0 1 0 7,Identity", { { 5, 0 }, { 0, 7 }, { 0, 0 } });
            VerifyChildren(L"1 0 0 1 3 4", { { 3, 4 } });
        });
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringHonorsQuotesAndEscapes()
    {
        TestCleanupWrapper cleanup;

        RunOnBothParserRoutes([&]()
        {
            // Quoted literals (single and double quotes) keep their commas, so each literal is one entry.
            VerifyChildren(L"'1,0,0,1,5,0',&quot;1,0,0,1,0,7&quot;", { { 5, 0 }, { 0, 7 } });

            // Brackets inside a quoted literal are accepted and do not split the entry. (The matrix converter ignores
            // text after six numbers, so this checks acceptance and entry boundaries, not the bracket text itself.)
            VerifyChildren(L"'1 0 0 1 2 0 [x]',1 0 0 1 0 3", { { 2, 0 }, { 0, 3 } });

            // Escaped commas join the entry: the offsets only come out right if each \, is unescaped to a comma.
            VerifyChildren(L"1\\,0\\,0\\,1\\,9\\,0", { { 9, 0 } });
            // Escaped brackets, quotes and backslash are accepted in plain text without splitting or ending the entry,
            // and an escaped ordinary character (\1) is unescaped (otherwise the matrix would not parse). The escaped
            // bracket/quote/backslash text itself is not observable because the converter ignores trailing text.
            VerifyChildren(L"1 0 0 1 3 0 \\[x\\],1 0 0 1 4 0 \\'q\\&quot;,1 0 0 1 6 0 \\\\,\\1 0 0 1 8 0", { { 3, 0 }, { 4, 0 }, { 6, 0 }, { 8, 0 } });

            // Inside quoted literals an escaped quote does not end the literal, and an escaped comma is unescaped
            // (the second entry only parses as a matrix if \, became a comma).
            VerifyChildren(L"'1 0 0 1 10 0 \\' \\,',&quot;1\\,0 0 1 0 11&quot;", { { 10, 0 }, { 0, 11 } });
        });
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringTrimsUnquotedWhitespace()
    {
        TestCleanupWrapper cleanup;

        RunOnBothParserRoutes([&]()
        {
            // Whitespace (including a tab character reference) around unquoted entries is trimmed.
            VerifyChildren(L"  1 0 0 1 5 0  ,&#9;Identity&#9;", { { 5, 0 }, { 0, 0 } });

            // Whitespace before and after quoted literals is skipped; content inside the quotes is kept.
            VerifyChildren(L"   '1 0 0 1 12 0'  ,  &quot; 1 0 0 1 0 13 &quot;  ", { { 12, 0 }, { 0, 13 } });
        });
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringWithInvalidSyntaxFailsToParse()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            const wchar_t* expected = L"Failed to parse initialization string";

            VerifyLoadFails(L"1 0 0 1 0 0, [3]", expected);              // unescaped reserved symbol in plain text
            VerifyLoadFails(L"Identity]", expected);                     // reserved symbol directly after text
            VerifyLoadFails(L"[Identity", expected);                     // reserved symbol at the start
            VerifyLoadFails(L"Identity,'Identity", expected);            // missing closing quote
            VerifyLoadFails(L"Identity, 'Identity''", expected);         // extra quote
            VerifyLoadFails(L"'Identity' 'Identity'", expected);         // no comma between entries
            VerifyLoadFails(L"'Identity',", expected);                   // comma at the end
            VerifyLoadFails(L"'Identity&quot;", expected);               // mismatched quote kinds
            VerifyLoadFails(L",Identity", expected);                     // leading comma
            VerifyLoadFails(L"Identity,,Identity", expected);            // empty entry between commas
            VerifyLoadFails(L"Identity\\", expected);                    // dangling escape at the end
            VerifyLoadFails(L"'Identity\\", expected);                   // dangling escape inside a literal
        });
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringWithBlankEntryFailsConversion()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            // A whitespace-only entry parses successfully but becomes an empty item, which cannot be converted.
            VerifyLoadFails(L" ", L"Failed to create a");
            VerifyLoadFails(L"Identity, ", L"Failed to create a");
        });
    }

    void CollectionInitializationStringIntegrationTests::ItemsInitializationStringEmptyValueFailsToParse()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            VerifyLoadFails(L"", L"Failed to parse initialization string");
        });
    }

} } } } } }
