// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TextBox {

    class TextBoxAutomationIntegrationTests : public WEX::TestClass<TextBoxAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(TextBoxAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(DoesSupportEssentialPatterns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates supported UIA patterns for TextBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates supported UIA patterns for TextBox.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTextPattern2English)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIA Text patterns selection and caret API, English Text.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTextPattern2BIDI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIA Text patterns selection and caret API, BIDI Text.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPlaceholderTextIsMovedToDescribedBy)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that placeholder text is moved to DescribedBy.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDescribedByIsNotClobberedByPlaceholderText)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that placeholder text is not moved to DescribedBy if the list is non-empty.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPlaceHolderTextNotMovedToDescribedByWhenTemplatePartIsMissing)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the placeholder text is not moved to DescribedBy when the relevant Template Part is missing.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
            
        BEGIN_TEST_METHOD(VerifyTextRangerProviderCompare)
            TEST_METHOD_PROPERTY(L"Description", L"Comparing two different text range providers should fail with E_INVALIDARG.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    private:
        void TextPattern2TestHelper(xaml_controls::TextBox^ textBox, const Automation::AutomationClient::UIAElementInfo& uiaInfo, int selectStart, int selectLength, const WCHAR *comparisonStr);
        Platform::String^ GetResourcesPath() const;
    };

} } } } } }
