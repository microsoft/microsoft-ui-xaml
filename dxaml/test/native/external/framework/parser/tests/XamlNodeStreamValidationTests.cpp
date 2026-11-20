// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <string>
#include "XamlNodeStreamValidationTests.h"

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
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
        // Loads markup that must be rejected and verifies the parse HRESULT plus every expected
        // fragment of the error message (the message names the offending type/member).
        void VerifyLoadFailsWithMessage(String^ markup, std::initializer_list<const wchar_t*> expectedFragments)
        {
            bool threw = false;
            HRESULT hr = S_OK;
            std::wstring message;
            try
            {
                XamlReader::Load(markup);
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

            VERIFY_IS_TRUE(threw, L"Invalid markup must not load.");
            VERIFY_ARE_EQUAL(c_xamlParseFailed, hr);
            LOG_OUTPUT(L"Error message: %s", message.c_str());
            for (auto fragment : expectedFragments)
            {
                VERIFY_IS_TRUE(message.find(fragment) != std::wstring::npos, fragment);
            }
        }
    }

    bool XamlNodeStreamValidationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool XamlNodeStreamValidationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool XamlNodeStreamValidationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void XamlNodeStreamValidationTests::UnknownElementReportsUnknownType()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            // Control: the same markup with a real element loads, so the failure is caused by the unknown type.
            auto grid = safe_cast<Grid^>(XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><Border /></Grid>"));
            VERIFY_ARE_EQUAL(1u, grid->Children->Size);

            VerifyLoadFailsWithMessage(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><NotARealType /></Grid>",
                { L"The type 'NotARealType' was not found" });
        });
    }

    void XamlNodeStreamValidationTests::UnknownElementInUnknownNamespaceReportsUnknownNamespace()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            VerifyLoadFailsWithMessage(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:bogus='http://example.invalid/not-a-xaml-namespace'>"
                L"  <bogus:Widget />"
                L"</Grid>",
                { L"Widget", L"http://example.invalid/not-a-xaml-namespace", L"unknown namespace" });
        });
    }

    void XamlNodeStreamValidationTests::UnknownAttributeReportsUnknownProperty()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            VerifyLoadFailsWithMessage(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' NotARealProperty='1' />",
                { L"The property 'NotARealProperty' was not found in type 'Grid'" });
        });
    }

    void XamlNodeStreamValidationTests::UnknownAttachedAttributeReportsUnknownAttachableProperty()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            // Control: a real attached property on the same owner loads.
            auto grid = safe_cast<Grid^>(XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Grid.Row='1' />"));
            VERIFY_ARE_EQUAL(1, Grid::GetRow(grid));

            VerifyLoadFailsWithMessage(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Grid.NotARealAttachedProperty='1' />",
                { L"The attachable property 'NotARealAttachedProperty' was not found in type 'Grid'" });
        });
    }

    void XamlNodeStreamValidationTests::ContentOnTypeWithoutContentPropertyReportsCannotAddContent()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnBothParserRoutes([&]()
        {
            VerifyLoadFailsWithMessage(
                // Text content after a real attribute. XamlPullParser rejects element content earlier with a different
                // error, and treats text with no preceding real property as text initialization of the type, so the
                // Width attribute is needed for the text to become unknown content that the node-stream validator reports.
                L"<Rectangle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Width='10'>hello</Rectangle>",
                { L"Cannot add content to an object of type", L"Rectangle" });
        });
    }

} } } } } }
