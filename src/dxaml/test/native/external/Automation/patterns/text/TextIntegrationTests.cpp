// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>

#include <XamlTailored.h>

#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <Patterns\MockTextPatternObject.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation::Patterns;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Text {

    bool TextIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool TextIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TextIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void TextIntegrationTests::VerifyRangeFromPoint()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestTextBlock";
        uiaInfo.m_AutomationID = L"TestTextBlock";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
            textBlock->Text = "Hello World";
            textBlock->FontSize = 30.0;
            TestServices::WindowHelper->WindowContent = textBlock;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ textBlockAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            AutoBSTR textFromTextRange;

            RunOnUIThread([&]()
            {
                textBlockAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBlock);
            });
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
            WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

            RECT screenRect{};
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentBoundingRectangle(&screenRect));
            LOG_OUTPUT(L"left %ld, right %ld, top %ld, bottom %ld \n", screenRect.left, screenRect.right, screenRect.top, screenRect.bottom);
            const POINT point = { screenRect.left + 50, screenRect.top + 50 };

            // Expansion test a line.
            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"Hello World", textFromTextRange));

            // Expansion test a word.
            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Word));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"Hello ", textFromTextRange));

            // Expansion test a character.
            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Character));
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"o", textFromTextRange));
        });
    }

    void TextIntegrationTests::VerifyRangeFromPointWithHiDPI()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBlock^ textBlock;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestTextBlock";
        uiaInfo.m_AutomationID = L"TestTextBlock";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, ref new Platform::String(uiaInfo.m_Name));
            textBlock->Text = "Hello World\r\nGoodbye planet";
            textBlock->FontSize = 10.0;
            textBlock->Margin = ThicknessHelper::FromLengths(100.0, 100.0, 0, 0);
            TestServices::WindowHelper->WindowContent = textBlock;
        });
        TestServices::WindowHelper->WaitForIdle();

        // We should get the second line of text without HiDPI
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ textBlockAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            AutoBSTR textFromTextRange;


            RunOnUIThread([&]()
            {
                textBlockAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBlock);
            });
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
            WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

            RECT screenRect{};
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentBoundingRectangle(&screenRect));
            LOG_OUTPUT(L"left %ld, right %ld, top %ld, bottom %ld \n", screenRect.left, screenRect.right, screenRect.top, screenRect.bottom);
            const POINT point = { screenRect.left + 50, screenRect.top + 50 };

            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            // Pass -1 for maxLength to get full string.
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"Goodbye planet", textFromTextRange));
        });

        // Now set HiDPI and make sure we get the first line of text
        TestServices::WindowHelper->ResetWindowContentAndScaleWaitForIdle(2.0);
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = textBlock;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            xaml_automation_peers::AutomationPeer^ textBlockAP;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            AutoBSTR textFromTextRange;

            RunOnUIThread([&]()
            {
                textBlockAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(textBlock);
            });
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"TextIntegrationTests::VerifyRangeFromPoint: Failed in retreiving Text Pattern.");
            WEX::Common::Throw::IfNull(spTextPattern.Get(), L"TextIntegrationTests::VerifyRangeFromPoint: This TextBlock doesn't support Text Pattern which is required.");

            RECT screenRect{};
            VERIFY_SUCCEEDED(spUIAutomationElement->get_CurrentBoundingRectangle(&screenRect));
            LOG_OUTPUT(L"left %ld, right %ld, top %ld, bottom %ld \n", screenRect.left, screenRect.right, screenRect.top, screenRect.bottom);
            const POINT point = { screenRect.left - 50, screenRect.top - 50 };

            VERIFY_SUCCEEDED(spTextPattern->RangeFromPoint(point, &spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUIAutomationTextRange->ExpandToEnclosingUnit(TextUnit_Line));
            // Pass -1 for maxLength to get full string.
            VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!wcscmp(L"Hello World\r\n", textFromTextRange));
        });
    }

    void TextIntegrationTests::VerifyGetCaretRange()
    {
        TestCleanupWrapper cleanup;

        MockUpProviderControlAutomationPeer^ mockAP;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"MockControl";

        RunOnUIThread([&]()
        {
            auto control = ref new MockUpProviderControl();
            mockAP = static_cast<MockUpProviderControlAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control));
            xaml_automation::AutomationProperties::SetName(control, ref new Platform::String(uiaInfo.m_Name));

            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(control);;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextPattern> spTextPattern;
            wrl::ComPtr<IUIAutomationTextPattern2> spTextPattern2;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern));
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &spTextPattern2));

            mockAP->MakeRangeValid(true);
            BOOL isActive;
            wrl::ComPtr<IUIAutomationTextRange> iuiautomationTextRange;

            // The Mock returns the same TextRangeProvider for everything. We call get_DocumentRange so we 
            // have something to compare the IUIAutomationTextRange returned by GetCaretRange against. 
            wrl::ComPtr<IUIAutomationTextRange> documentRange;
            VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&documentRange));

            mockAP->GetCaretRangeIsActiveMockValue = true;
            VERIFY_SUCCEEDED(spTextPattern2->GetCaretRange(&isActive, iuiautomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(!!isActive);
            VERIFY_IS_NOT_NULL(iuiautomationTextRange);
            BOOL areEqual;
            VERIFY_SUCCEEDED(iuiautomationTextRange->Compare(documentRange.Get(), &areEqual));
            VERIFY_IS_TRUE(!!areEqual);

            mockAP->GetCaretRangeIsActiveMockValue = false;
            mockAP->MakeRangeValid(false);
            VERIFY_SUCCEEDED(spTextPattern2->GetCaretRange(&isActive, iuiautomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_FALSE(!!isActive);
            VERIFY_IS_NULL(iuiautomationTextRange);
        });
    }
} } } } } }
