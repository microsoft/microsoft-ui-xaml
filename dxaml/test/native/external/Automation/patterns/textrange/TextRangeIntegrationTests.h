// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace TextRange {

    class TextRangeIntegrationTests : public WEX::TestClass<TextRangeIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(TextRangeIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyGetAttributeValue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the availibility of text attribute ids with a textrange provider method GetAttribute.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFindAttribute)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the availibility of text attribute ids with a textrange provider method FindAttribute.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                // WPF_HOSTING_MODE_FAILURE: In WPF-hosting mode, we crash in the dtor of an AutoVariant:                
                // 04 ntdll!LdrpDispatchUserCallTarget
                // 05 oleaut32!ReleaseResources
                // 06 oleaut32!_SafeArrayDestroy
                // 07 oleaut32!SafeArrayDestroy
                // 08 oleaut32!VariantClearWorker
                // 09 oleaut32!VariantClear
                // 0a Microsoft_UI_Xaml_Tests_External_Automation!Microsoft::UI::Xaml::Tests::Common::AutoVariant::~AutoVariant
                // 0b Microsoft_UI_Xaml_Tests_External_Automation!Microsoft::UI::Xaml::Tests::Automation::TextRange::VerifyFindAttributeAPArray
                // 0c Microsoft_UI_Xaml_Tests_External_Automation!<lambda_25388ab5f2fcefd7bdd77bf989d8cd8e>::operator()
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") 
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(TestCompare)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can call ITextRangeProvider::Compare correctly")
        END_TEST_METHOD()
    };

} } } } } }
