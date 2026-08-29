// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TextBox {

    bool TextBoxAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TextBoxAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TextBoxAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    Platform::String^ TextBoxAutomationIntegrationTests::GetResourcesPath() const
    {
        return GetPackageFolder() + L"resources\\native\\controls\\textbox\\";
    }

    //
    // Test Cases
    //
    void TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ textBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TextBoxControl";
        uiaInfo.m_AutomationID = L"TextBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            textBox = ref new xaml_controls::TextBox();
            xaml_automation::AutomationProperties::SetName(textBox, ref new Platform::String(uiaInfo.m_Name));
            textBox->Name = "TextBoxControl";
            textBox->Text = "1234";
            textBox->FontSize = 30.0;
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::TextBox>(textBox);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextEditPattern> spUITextEditPattern;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;
            wrl::ComPtr<IUIAutomationTextPattern2> spUITextPattern2;
            wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;
            AutoBSTR textFromTextRange;

            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Executing test on UI thread");
                auto textBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBox);
                VERIFY_IS_NOT_NULL(textBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            WEX::Logging::Log::Comment(L"Test TextEdit pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextEditPatternId, __uuidof(IUIAutomationTextEditPattern), &spUITextEditPattern), L" TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns.");
            WEX::Common::Throw::IfNull(spUITextEditPattern.Get(), L" TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns: This element doesn't support TextEdit Pattern which is required.");
            VERIFY_IS_NOT_NULL(spUITextEditPattern.Get());

            WEX::Logging::Log::Comment(L"Test Text pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern), L" TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns: Failed in fetching Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L" TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns: This element doesn't support Text Pattern which is required.");
            VERIFY_IS_NOT_NULL(spUITextPattern.Get());

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUITextPattern->GetVisibleRanges(spUITextArrangeArray.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());

            VERIFY_SUCCEEDED(spUITextPattern->GetSelection(spUITextArrangeArray.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());

            VERIFY_SUCCEEDED(spUITextArrangeArray->GetElement(0, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"1234", textFromTextRange));

            WEX::Logging::Log::Comment(L"Test Text2 pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &spUITextPattern2), L" TextBoxAutomationIntegrationTests::DoesSupportEssentialPatterns: Failed in fetching Text2 Pattern.");
            VERIFY_IS_NOT_NULL(spUITextPattern2.Get());
            VERIFY_FAILED(spUITextPattern2->GetCaretRange(nullptr, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            BOOL isActive = FALSE;
            VERIFY_SUCCEEDED(spUITextPattern2->GetCaretRange(&isActive, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
        });
    }

    void TextBoxAutomationIntegrationTests::TextPattern2TestHelper(xaml_controls::TextBox^ textBox, const Automation::AutomationClient::UIAElementInfo& uiaInfo, int selectStart, int selectLength, const WCHAR *comparisonStr)
    {
        RunOnUIThread([&]()
        {
            textBox->Select(selectStart, selectLength);
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern2> spUITextPattern2;
            AutoBSTR textFromTextRange;

            RunOnUIThread([&]()
            {
                WEX::Logging::Log::Comment(L"Executing test on UI thread");
                auto textBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBox);
                VERIFY_IS_NOT_NULL(textBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &spUITextPattern2), L"Failed in fetching Text2 Pattern.");
            VERIFY_IS_NOT_NULL(spUITextPattern2.Get());

            BOOL isActive = FALSE;
            VERIFY_SUCCEEDED(spUITextPattern2->GetCaretRange(&isActive, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!!isActive);
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            if (textFromTextRange) // caret range should always comes back collapsed
            {
                VERIFY_ARE_EQUAL(wcslen(textFromTextRange), static_cast<size_t>(0));
            }
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Character));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            LOG_OUTPUT(L"=========Comparing text==========");

            if (comparisonStr != nullptr)
            {
                LOG_OUTPUT(L"Expected Text=%ws", comparisonStr);
            }
            else
            {
                LOG_OUTPUT(L"Expected Text is null or blank");
            }

            if (textFromTextRange)
            {
                LOG_OUTPUT(L"Actual Text from UIAutomationTextRange=%ws", textFromTextRange.Get());
            }
            else
            {
                LOG_OUTPUT(L"Text from UIAutomationTextRange is null");
            }

            if (comparisonStr != nullptr)
            {
                VERIFY_IS_TRUE(!wcscmp(comparisonStr, textFromTextRange));
            }
            else
            {
                if (textFromTextRange) //if not null, it has to be blank string
                {
                    VERIFY_ARE_EQUAL(wcslen(textFromTextRange), static_cast<size_t>(0));
                }
            }
        });
    }

    void TextBoxAutomationIntegrationTests::VerifyTextPattern2English()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ textBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TextBoxControl";
        uiaInfo.m_AutomationID = L"TextBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            textBox = ref new xaml_controls::TextBox();
            xaml_automation::AutomationProperties::SetName(textBox, ref new Platform::String(uiaInfo.m_Name));
            textBox->Name = "TextBoxControl";
            textBox->FontSize = 30.0;
            textBox->AcceptsReturn = true;
            textBox->TextWrapping = TextWrapping::Wrap;
            textBox->Text = "ABCDE\r\nF";
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::TextBox>(textBox);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox->Focus(FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        TextPattern2TestHelper(textBox, uiaInfo, 0, 0, L"A"); // CP at position 0, after expand it should contain text range of 'A'
        TextPattern2TestHelper(textBox, uiaInfo, 1, 0, L"B"); // CP at position 1, after expand it should contain text range of 'B'
        TextPattern2TestHelper(textBox, uiaInfo, 4, 0, L"E"); // CP at position 4, after expand it should contain text range of 'E'
        TextPattern2TestHelper(textBox, uiaInfo, 6, 0, L"F"); // CP at position 6, after expand it should contain text range of 'F' of next line
        TextPattern2TestHelper(textBox, uiaInfo, 7, 0, nullptr); // CP at position 7, after expand it should contain null text
        TextPattern2TestHelper(textBox, uiaInfo, 0, 1, L"B"); // First char selected, GetCaretRange should collapse the selection; After expand it should contain text 'B'
        TextPattern2TestHelper(textBox, uiaInfo, 3, 1, L"E"); // 'D' selected, GetCaretRange should collapse the selection; After expand it should contain text 'E'
        TextPattern2TestHelper(textBox, uiaInfo, 6, 1, nullptr); // Last char selected, GetCaretRange should collapse the selection; After expand it should contain null text
        TextPattern2TestHelper(textBox, uiaInfo, 0, 7, nullptr); // All Text selected, GetCaretRange should collapse the selection; After expand it should contain null text
    }

    void TextBoxAutomationIntegrationTests::VerifyTextPattern2BIDI()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ textBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TextBoxControl";
        uiaInfo.m_AutomationID = L"TextBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            textBox = ref new xaml_controls::TextBox();
            xaml_automation::AutomationProperties::SetName(textBox, ref new Platform::String(uiaInfo.m_Name));
            textBox->Name = "TextBoxControl";
            textBox->FontSize = 30.0;
            
            // U+0646 and U+0633 are Arabic characters that trigger bidi functionality.
            textBox->Text = L"\u0646\u06331!";
            
            textBox->TextReadingOrder = TextReadingOrder::DetectFromContent;
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::TextBox>(textBox);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            textBox->Focus(FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        TextPattern2TestHelper(textBox, uiaInfo, 0, 0, L"\u0646"); // CP at position 0, after expand it should contain text range of first BIDI char
        TextPattern2TestHelper(textBox, uiaInfo, 1, 0, L"\u0633"); // CP at position 1, after expand it should contain text range of second BIDI char
        TextPattern2TestHelper(textBox, uiaInfo, 3, 0, L"!"); // CP at position 3, after expand it should contain text range of !
        TextPattern2TestHelper(textBox, uiaInfo, 4, 0, nullptr); // CP at position 4, after expand it should contain null text
        TextPattern2TestHelper(textBox, uiaInfo, 0, 1, L"\u0633"); // First char selected, GetCaretRange should collapse the selection; After expand it should contain second BIDI char
        TextPattern2TestHelper(textBox, uiaInfo, 2, 1, L"!"); // last bidi char selected, GetCaretRange should collapse the selection; After expand it should contain text '!'
        TextPattern2TestHelper(textBox, uiaInfo, 3, 1, nullptr); // Last char selected, GetCaretRange should collapse the selection; After expand it should contain null text
        TextPattern2TestHelper(textBox, uiaInfo, 0, 4, nullptr); // All Text selected, GetCaretRange should collapse the selection; After expand it should contain null text
    }

    void TextBoxAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TextBox with Name";

        xaml_controls::TextBox^ TextBoxWithName = nullptr;
        xaml_controls::TextBox^ TextBoxWithHeader = nullptr;
        xaml_controls::TextBox^ TextBoxWithPlaceholderText = nullptr;
        xaml_controls::TextBox^ TextBoxWithNoAccName = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        // Setup
        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            TextBoxWithName = ref new xaml_controls::TextBox();
            TextBoxWithName->Header = "TextBox with Name - Header";
            xaml_automation::AutomationProperties::SetName(TextBoxWithName, ref new Platform::String(L"TextBox with Name"));
            rootPanel->Children->Append(TextBoxWithName);

            TextBoxWithHeader = ref new xaml_controls::TextBox();
            TextBoxWithHeader->Header = "TextBox with Header";
            rootPanel->Children->Append(TextBoxWithHeader);

            TextBoxWithPlaceholderText = ref new xaml_controls::TextBox();
            TextBoxWithPlaceholderText->PlaceholderText = "TextBox with PlaceholderText";
            rootPanel->Children->Append(TextBoxWithPlaceholderText);

            TextBoxWithNoAccName = ref new xaml_controls::TextBox();
            rootPanel->Children->Append(TextBoxWithNoAccName);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spTextBoxWithName;
            wrl::ComPtr<IUIAutomationElement> spTextBoxWithHeader;
            wrl::ComPtr<IUIAutomationElement> spTextBoxWithPlaceholderText;
            wrl::ComPtr<IUIAutomationElement> spTextBoxWithNoAccName;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spTextBoxWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for TextBoxWithName exists.");
            VERIFY_IS_NOT_NULL(spTextBoxWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spTextBoxWithName.");
            spTextBoxWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second TextBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBoxWithName.Get(), &spTextBoxWithHeader);
            VERIFY_IS_NOT_NULL(spTextBoxWithHeader);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spTextBoxWithHeader.");
            spTextBoxWithHeader->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"TextBox with Header", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third TextBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBoxWithHeader.Get(), &spTextBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(spTextBoxWithPlaceholderText);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spTextBoxWithPlaceholderText.");
            spTextBoxWithPlaceholderText->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"TextBox with PlaceholderText", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the fourth TextBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spTextBoxWithPlaceholderText.Get(), &spTextBoxWithNoAccName);
            VERIFY_IS_NOT_NULL(spTextBoxWithNoAccName);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spTextBoxWithNoName.");
            spTextBoxWithNoAccName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
        });
    }

    void TextBoxAutomationIntegrationTests::VerifyPlaceholderTextIsMovedToDescribedBy()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ TextBoxWithPlaceholderText = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_automation_peers::AutomationPeer^ TextBoxWithPlaceholderTextAP = nullptr;
        Platform::String^ const placeHolderText = "TextBox with PlaceholderText";

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            TextBoxWithPlaceholderText = ref new xaml_controls::TextBox();
            TextBoxWithPlaceholderText->Name = "txb1";
            TextBoxWithPlaceholderTextAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(TextBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(TextBoxWithPlaceholderTextAP);

            rootPanel->Children->Append(TextBoxWithPlaceholderText);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            TextBoxWithPlaceholderText->PlaceholderText = placeHolderText;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto describedByVector = TextBoxWithPlaceholderTextAP->GetDescribedByCore()->First();
            auto textBlockAP = (xaml_automation_peers::FrameworkElementAutomationPeer^)describedByVector->Current;
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::TextBlockAutomationPeer^>(textBlockAP));
            auto placeHolderTextBlock = static_cast<xaml_controls::TextBlock^>(textBlockAP->Owner);
            VERIFY_ARE_EQUAL(placeHolderText, placeHolderTextBlock->Text);

            LOG_OUTPUT(L"Verify that only one DescribedBy property was set.");
            VERIFY_IS_FALSE(describedByVector->MoveNext());
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Entering text into the TextBox.");
            TextBoxWithPlaceholderText->Text = "Lorem Ipsum";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verify that the DescribedBy property no longer holds placeholder text.");
            auto describedBy = TextBoxWithPlaceholderTextAP->GetDescribedByCore();
            VERIFY_IS_NULL(describedBy);
        });
    }

    void TextBoxAutomationIntegrationTests::VerifyDescribedByIsNotClobberedByPlaceholderText()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ TextBoxWithPlaceholderText = nullptr;
        xaml_controls::Button^ DescribedByButton = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_automation_peers::AutomationPeer^ TextBoxWithPlaceholderTextAP = nullptr;
        Platform::String^ const placeHolderText = "TextBox with PlaceholderText";

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            TextBoxWithPlaceholderText = ref new xaml_controls::TextBox();
            DescribedByButton = ref new xaml_controls::Button();

            xaml_automation::AutomationProperties::SetName(TextBoxWithPlaceholderText, ref new Platform::String(L"txb1"));
            TextBoxWithPlaceholderTextAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(TextBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(TextBoxWithPlaceholderTextAP);

            rootPanel->Children->Append(TextBoxWithPlaceholderText);
            rootPanel->Children->Append(DescribedByButton);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto describedByVector = xaml_automation::AutomationProperties::GetDescribedBy(TextBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(describedByVector);
            describedByVector->Append(DescribedByButton);

            TextBoxWithPlaceholderText->PlaceholderText = placeHolderText;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Verify that the first element of the DescribedBy properties contains the DescribedByButton.");
            auto describedByVector = TextBoxWithPlaceholderTextAP->GetDescribedByCore()->First();
            auto ButtonAP = (xaml_automation_peers::FrameworkElementAutomationPeer^)describedByVector->Current;
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::ButtonAutomationPeer^>(ButtonAP));
            VERIFY_ARE_EQUAL(DescribedByButton, static_cast<xaml_controls::Button^>(ButtonAP->Owner));

            VERIFY_IS_TRUE(describedByVector->MoveNext());

            LOG_OUTPUT(L"Verify that the second element of the DescribedBy properties contains the PlaceHoldertext.");
            auto textBlockAP = (xaml_automation_peers::FrameworkElementAutomationPeer^)describedByVector->Current;
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::TextBlockAutomationPeer^>(textBlockAP));
            auto placeHolderTextBlock = static_cast<xaml_controls::TextBlock^>(textBlockAP->Owner);
            VERIFY_ARE_EQUAL(placeHolderText, placeHolderTextBlock->Text);

            LOG_OUTPUT(L"Verify that only two DescribedBy properties were set.");
            VERIFY_IS_FALSE(describedByVector->MoveNext());
        });
    }

    void TextBoxAutomationIntegrationTests::VerifyPlaceHolderTextNotMovedToDescribedByWhenTemplatePartIsMissing()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ TextBoxWithPlaceholderText = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_automation_peers::AutomationPeer^ TextBoxWithPlaceholderTextAP = nullptr;
        Platform::String^ const placeHolderText = "TextBox with PlaceholderText";

        auto retemplatedResources = safe_cast<Microsoft::UI::Xaml::ResourceDictionary^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TextBoxWithDifferentPlaceholderTextControl.xaml"));

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            rootPanel->Resources = retemplatedResources;

            TextBoxWithPlaceholderText = ref new xaml_controls::TextBox();
            TextBoxWithPlaceholderText->Name = "txb1";
            TextBoxWithPlaceholderText->PlaceholderText = placeHolderText;

            TextBoxWithPlaceholderTextAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(TextBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(TextBoxWithPlaceholderTextAP);
            rootPanel->Children->Append(TextBoxWithPlaceholderText);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto describedByVector = TextBoxWithPlaceholderTextAP->GetDescribedByCore();
            VERIFY_IS_NULL(describedByVector);
        });
    }

    void TextBoxAutomationIntegrationTests::VerifyTextRangerProviderCompare()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::TextBlock^ textBlock = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfoTextBox;
        uiaInfoTextBox.m_Name = L"TextBoxControl";
        uiaInfoTextBox.m_AutomationID = L"TextBoxControl";
        uiaInfoTextBox.m_cType = UIA_TextControlTypeId;

        Automation::AutomationClient::UIAElementInfo uiaInfoTextBlock;
        uiaInfoTextBlock.m_Name = L"TextBlockControl";
        uiaInfoTextBlock.m_AutomationID = L"TextBlockControl";
        uiaInfoTextBlock.m_cType = UIA_TextControlTypeId;

        xaml_controls::StackPanel^ rootPanel = nullptr;

        // Setup
        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();
            textBox = ref new xaml_controls::TextBox();
            xaml_automation::AutomationProperties::SetName(textBox, ref new Platform::String(uiaInfoTextBox.m_Name));
            textBox->Name = "TextBoxControl";
            textBox->Text = "1234";
            textBox->FontSize = 30.0;
            rootPanel->Children->Append(textBox);

            textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfoTextBlock.m_Name));
            textBox->Name = "TextBlockControl";
            textBox->Text = "5678";
            textBox->FontSize = 30.0;
            rootPanel->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = rootPanel;

        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementForTextBox;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRangeForTextBox;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPatternForTextBox;

            wrl::ComPtr<IUIAutomationElement> spUIAutomationElementForTextBlock;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRangeForTextBlock;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPatternForTextBlock;

            RunOnUIThread([&]()
            {
                auto textBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBox);
                VERIFY_IS_NOT_NULL(textBoxAsAutomationPeer);
                auto textBlockAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBlock);
                VERIFY_IS_NOT_NULL(textBlockAsAutomationPeer);
            });

            auto spAutomationClientManagerForTextBox = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTextBox);
            spAutomationClientManagerForTextBox->GetCurrentUIAutomationElement(&spUIAutomationElementForTextBox);
            VERIFY_IS_NOT_NULL(spUIAutomationElementForTextBox.Get());
            VERIFY_SUCCEEDED(spUIAutomationElementForTextBox->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPatternForTextBox));
            VERIFY_IS_NOT_NULL(spUITextPatternForTextBox.Get());
            VERIFY_SUCCEEDED(spUITextPatternForTextBox->get_DocumentRange(spUIAutomationTextRangeForTextBox.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRangeForTextBox.Get());

            auto spAutomationClientManagerForTextBlock = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfoTextBlock);
            spAutomationClientManagerForTextBlock->GetCurrentUIAutomationElement(&spUIAutomationElementForTextBlock);
            VERIFY_IS_NOT_NULL(spUIAutomationElementForTextBlock.Get());
            VERIFY_SUCCEEDED(spUIAutomationElementForTextBlock->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPatternForTextBlock));
            VERIFY_IS_NOT_NULL(spUITextPatternForTextBlock.Get());
            VERIFY_SUCCEEDED(spUITextPatternForTextBlock->get_DocumentRange(spUIAutomationTextRangeForTextBlock.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRangeForTextBlock.Get());

            BOOL areSame;
            VERIFY_ARE_EQUAL(spUIAutomationTextRangeForTextBlock->Compare(spUIAutomationTextRangeForTextBox.Get(), &areSame), E_INVALIDARG);
            VERIFY_IS_FALSE(areSame);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::TextBox
