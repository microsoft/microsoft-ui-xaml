// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichEditBoxAutomationIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>

#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationGenericTests.h>
#include <TreeHelper.h>
#include "FileLoader.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace concurrency;
using namespace ::Windows::Storage;
using namespace ::Windows::Foundation;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichEditBox {

    bool RichEditBoxAutomationIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RichEditBoxAutomationIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RichEditBoxAutomationIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void RichEditBoxAutomationIntegrationTests::VerifyDefaultAutomationName()
    {
        TestCleanupWrapper cleanup;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"RichEditBox with Name";

        xaml_controls::RichEditBox^ RichEditBoxWithName = nullptr;
        xaml_controls::RichEditBox^ RichEditBoxWithHeader = nullptr;
        xaml_controls::RichEditBox^ RichEditBoxWithPlaceholderText = nullptr;
        xaml_controls::RichEditBox^ RichEditBoxWithNoAccName = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;

        // Setup
        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            RichEditBoxWithName = ref new xaml_controls::RichEditBox();
            RichEditBoxWithName->Header = "RichEditBox with Name - Header";
            xaml_automation::AutomationProperties::SetName(RichEditBoxWithName, ref new Platform::String(L"RichEditBox with Name"));
            rootPanel->Children->Append(RichEditBoxWithName);

            RichEditBoxWithHeader = ref new xaml_controls::RichEditBox();
            RichEditBoxWithHeader->Header = "RichEditBox with Header";
            rootPanel->Children->Append(RichEditBoxWithHeader);

            RichEditBoxWithPlaceholderText = ref new xaml_controls::RichEditBox();
            RichEditBoxWithPlaceholderText->PlaceholderText = "RichEditBox with PlaceholderText";
            rootPanel->Children->Append(RichEditBoxWithPlaceholderText);

            RichEditBoxWithNoAccName = ref new xaml_controls::RichEditBox();
            rootPanel->Children->Append(RichEditBoxWithNoAccName);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spRichEditBoxWithName;
            wrl::ComPtr<IUIAutomationElement> spRichEditBoxWithHeader;
            wrl::ComPtr<IUIAutomationElement> spRichEditBoxWithPlaceholderText;
            wrl::ComPtr<IUIAutomationElement> spRichEditBoxWithNoAccName;
            wrl::ComPtr<IUIAutomationTreeWalker> spUIAutomationTreeWalker;
            wrl::ComPtr<IUIAutomationCondition> spUIAutomationTrueCondition;
            wrl::ComPtr<IUIAutomation> spUIAutomation;
            Common::AutoVariant autoVar;

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);

            spAutomationClientManager->GetAutomation(&spUIAutomation);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spRichEditBoxWithName);

            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTrueCondition(&spUIAutomationTrueCondition), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating True PropertyCondition.");
            LogThrow_IfFailedWithMessage(spUIAutomation->CreateTreeWalker(spUIAutomationTrueCondition.Get(), &spUIAutomationTreeWalker), L"ListBoxAutomationIntegrationTests::VerifySizePosLevelFromMarkup: Failed in creating TreeWalker.");

            LOG_OUTPUT(L"Verifying UIA Client side node for RichEditBoxWithName exists.");
            VERIFY_IS_NOT_NULL(spRichEditBoxWithName);

            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spRichEditBoxWithName.");
            spRichEditBoxWithName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(uiaInfo.m_Name, (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the second RichEditBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spRichEditBoxWithName.Get(), &spRichEditBoxWithHeader);
            VERIFY_IS_NOT_NULL(spRichEditBoxWithHeader);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spRichEditBoxWithHeader.");
            spRichEditBoxWithHeader->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"RichEditBox with Header", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the third RichEditBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spRichEditBoxWithHeader.Get(), &spRichEditBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(spRichEditBoxWithPlaceholderText);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spRichEditBoxWithPlaceholderText.");
            spRichEditBoxWithPlaceholderText->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"RichEditBox with PlaceholderText", (autoVar.Storage())->bstrVal));

            LOG_OUTPUT(L"Navigate to the fourth RichEditBox.");
            spUIAutomationTreeWalker->GetNextSiblingElement(spRichEditBoxWithPlaceholderText.Get(), &spRichEditBoxWithNoAccName);
            VERIFY_IS_NOT_NULL(spRichEditBoxWithNoAccName);
            LOG_OUTPUT(L"Verifying UIA Name property from Client side node for spRichEditBoxWithNoName.");
            spRichEditBoxWithNoAccName->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.ReleaseAndGetAddressOf());
            VERIFY_IS_TRUE(!wcscmp(L"", (autoVar.Storage())->bstrVal));
        });
    }

    void RichEditBoxAutomationIntegrationTests::VerifyRichEditBoxPlaceholderTextIsMovedToDescribedBy()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RichEditBox^ RichEditBoxWithPlaceholderText = nullptr;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_automation_peers::AutomationPeer^ RichEditBoxWithPlaceholderTextAP = nullptr;
        Platform::String^ const placeHolderText = "RichEditBox with PlaceholderText";

        RunOnUIThread([&]()
        {
            rootPanel = ref new xaml_controls::StackPanel();

            RichEditBoxWithPlaceholderText = ref new xaml_controls::RichEditBox();
            RichEditBoxWithPlaceholderText->Name = "txb1";
            RichEditBoxWithPlaceholderTextAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(RichEditBoxWithPlaceholderText);
            VERIFY_IS_NOT_NULL(RichEditBoxWithPlaceholderTextAP);

            rootPanel->Children->Append(RichEditBoxWithPlaceholderText);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            RichEditBoxWithPlaceholderText->PlaceholderText = placeHolderText;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto describedByVector = RichEditBoxWithPlaceholderTextAP->GetDescribedByCore()->First();
            auto textBlockAP = (xaml_automation_peers::FrameworkElementAutomationPeer^)describedByVector->Current;
            VERIFY_IS_NOT_NULL(dynamic_cast<xaml_automation_peers::TextBlockAutomationPeer^>(textBlockAP));
            auto placeHolderTextBlock = static_cast<xaml_controls::TextBlock^>(textBlockAP->Owner);
            VERIFY_ARE_EQUAL(placeHolderText, placeHolderTextBlock->Text);

            LOG_OUTPUT(L"Verify that only one DescribedBy property was set.");
            VERIFY_IS_FALSE(describedByVector->MoveNext());
        });
    }

    void RichEditBoxAutomationIntegrationTests::MoveEndpoint()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RichEditBox^ reBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"RichEditBoxControl";
        uiaInfo.m_AutomationID = L"RichEditBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            reBox = ref new xaml_controls::RichEditBox();
            xaml_automation::AutomationProperties::SetName(reBox, ref new Platform::String(uiaInfo.m_Name));
            reBox->Name = L"RichEditBoxControl";
            reBox->Document->SetText(mut::TextSetOptions::None, L"Hello World");
            reBox->Document->Selection->StartPosition = 0;
            reBox->Document->Selection->EndPosition = 0;
            reBox->FontSize = 30.0;
            LOG_OUTPUT(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::RichEditBox>(reBox);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;
            wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Executing test on UI thread");
                auto richeditBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(reBox);
                VERIFY_IS_NOT_NULL(richeditBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LOG_OUTPUT(L"Get Text pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern), L" TextBoxAutomationIntegrationTests::MoveEndpoint : Failed in fetching Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L" TextBoxAutomationIntegrationTests::MoveEndpoint : This element doesn't support Text Pattern which is required.");
            VERIFY_IS_NOT_NULL(spUITextPattern.Get());

            VERIFY_SUCCEEDED(spUITextPattern->GetSelection(spUITextArrangeArray.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());

            VERIFY_SUCCEEDED(spUITextArrangeArray->GetElement(0, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            int moveCount = 0;
            LOG_OUTPUT(L"MoveEndpointByUnit, move the end EP to the end of line by two words.");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Word, 2, &moveCount));
            VERIFY_ARE_EQUAL(moveCount, 2);
            VerifyRangeText(spUIAutomationTextRange, L"Hello World");

            LOG_OUTPUT(L"MoveEndpointByRange, move the start EP to the end EP, the range should be right before EOP.");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_Start, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_End));
            LOG_OUTPUT(L"MoveEndpointByUnit, move the end EP by one character, the adjusted move count should be 1 for the EOP.");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &moveCount));
            LOG_OUTPUT(L"MoveEndpointByUnit, move the end EP by one character, the adjusted move count should be zero since it already reaches the end.");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &moveCount));
            VERIFY_ARE_EQUAL(moveCount, 0);
            LOG_OUTPUT(L"MoveEndpointByUnit, move the end EP by one character one more time, the adjusted move count should still be zero.");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &moveCount));
            VERIFY_ARE_EQUAL(moveCount, 0);
        });
    }

    void RichEditBoxAutomationIntegrationTests::TextRangeMove()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RichEditBox^ reBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"RichEditBoxControl";
        uiaInfo.m_AutomationID = L"RichEditBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            reBox = ref new xaml_controls::RichEditBox();
            xaml_automation::AutomationProperties::SetName(reBox, ref new Platform::String(uiaInfo.m_Name));
            reBox->Name = L"RichEditBoxControl";
            reBox->Document->SetText(mut::TextSetOptions::None, L"01 23\r 456 7 \r8 9");
            reBox->Document->Selection->StartPosition = 0;
            reBox->Document->Selection->EndPosition = 0;
            reBox->FontSize = 30.0;
            LOG_OUTPUT(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::RichEditBox>(reBox);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Executing test on UI thread");
                auto richeditBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(reBox);
                VERIFY_IS_NOT_NULL(richeditBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LOG_OUTPUT(L"Test Text pattern");
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern));
            VERIFY_IS_NOT_NULL(spUITextPattern.Get());

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            LOG_OUTPUT(L"MoveEndPointByRange: range end -> range start");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_End, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_Start));

            int countMoved = 0;
            LOG_OUTPUT(L"MoveEndpointByUnit: range end by 1");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"0");

            LOG_OUTPUT(L"Move forward one character");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"1");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"23");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"\r");

            LOG_OUTPUT(L"Move forward one line");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Line, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L" 456 7 \r");

            LOG_OUTPUT(L"Move forward one line");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Line, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"8 9");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"9");

            LOG_OUTPUT(L"Move forward one word, it should stop moving");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 0);

            LOG_OUTPUT(L"Move forward one char, it should stop moving");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 0);

            LOG_OUTPUT(L"Move backward two chars");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, -2, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, -2);
            VerifyRangeText(spUIAutomationTextRange, L"8");

            LOG_OUTPUT(L"Move backward three words");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -3, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, -3);
            VerifyRangeText(spUIAutomationTextRange, L"456 ");

            LOG_OUTPUT(L"Move backward one line");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Line, -1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, -1);
            VerifyRangeText(spUIAutomationTextRange, L"01 23\r");

            LOG_OUTPUT(L"Move backward one word, it should stop moving");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, -1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 0);

            LOG_OUTPUT(L"Move backward one char, it should stop moving");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Character, -1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 0);
        });
    }

    void RichEditBoxAutomationIntegrationTests::TextRangeMoveOverLinkAndObject()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RichEditBox^ reBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"RichEditBoxControl";
        uiaInfo.m_AutomationID = L"RichEditBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            reBox = ref new xaml_controls::RichEditBox();
            xaml_automation::AutomationProperties::SetName(reBox, ref new Platform::String(uiaInfo.m_Name));
            reBox->Name = L"RichEditBoxControl";
            reBox->Document->SetText(mut::TextSetOptions::None, L"one two link three");
            reBox->Document->Selection->StartPosition = 8;
            reBox->Document->Selection->EndPosition = 12;
            reBox->Document->Selection->Link = "\"http://msn.com\"";
            reBox->FontSize = 20.0;
            LOG_OUTPUT(L"Adding control to UI");
        });
        TreeHelper::AddElementIntoLivetree<xaml_controls::RichEditBox>(reBox);
        TestServices::WindowHelper->WaitForIdle();

        auto pBitmapInsertedEvent = std::make_shared<Event>();
        create_task(StorageFile::GetFileFromPathAsync(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\Smiley.bmp"))
            .then([=](StorageFile^ pFile)
        {
            VERIFY_IS_NOT_NULL(pFile);

            create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                .then([=](IRandomAccessStream^ pFileStream)
            {
                VERIFY_IS_NOT_NULL(pFileStream);

                RunOnUIThread([&]()
                {
                    auto range = reBox->Document->GetRange(3, 3);
                    range->InsertImage(20, 20, 0, Microsoft::UI::Text::VerticalCharacterAlignment::Baseline, "stuff", pFileStream);
                    pBitmapInsertedEvent->Set();
                });
            });
        });

        pBitmapInsertedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Executing test on UI thread");
                auto richeditBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(reBox);
                VERIFY_IS_NOT_NULL(richeditBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LOG_OUTPUT(L"Test Text pattern");
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern));
            VERIFY_IS_NOT_NULL(spUITextPattern.Get());

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            LOG_OUTPUT(L"MoveEndPointByRange: range end -> range start");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByRange(TextPatternRangeEndpoint_End, spUIAutomationTextRange.Get(), TextPatternRangeEndpoint_Start));

            int countMoved = 0;
            LOG_OUTPUT(L"MoveEndpointByUnit: range end by 1");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"o");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"stuff");

            LOG_OUTPUT(L"Move forward two word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 2, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 2);
            VerifyRangeText(spUIAutomationTextRange, L"two ");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L"link");

            LOG_OUTPUT(L"Move forward one word");
            VERIFY_SUCCEEDED(spUIAutomationTextRange->Move(TextUnit_Word, 1, &countMoved));
            VERIFY_ARE_EQUAL(countMoved, 1);
            VerifyRangeText(spUIAutomationTextRange, L" three");
        });
    }

    void RichEditBoxAutomationIntegrationTests::VerifyRangeText(wrl::ComPtr<IUIAutomationTextRange>& spUIAutomationTextRange, LPCWSTR expectedText)
    {
        AutoBSTR textFromRange;
        VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromRange.ReleaseAndGetAddressOf()));
        LOG_OUTPUT(L"Expected text:[%ws], range text:[%ws]", expectedText, textFromRange.Get());
        VERIFY_IS_TRUE(!wcscmp(expectedText, textFromRange));
    }

    void RichEditBoxAutomationIntegrationTests::GetVisibleRangesOnRTLBox()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::RichEditBox^ reBox = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"RichEditBoxControl";
        uiaInfo.m_AutomationID = L"RichEditBoxControl";
        uiaInfo.m_cType = UIA_TextControlTypeId;
        RunOnUIThread([&]()
        {
            reBox = ref new xaml_controls::RichEditBox();
            xaml_automation::AutomationProperties::SetName(reBox, ref new Platform::String(uiaInfo.m_Name));
            reBox->Name = L"RichEditBoxControl";
            reBox->Document->SetText(mut::TextSetOptions::None, L"Hello World");
            reBox->Document->Selection->StartPosition = 0;
            reBox->Document->Selection->EndPosition = 0;
            reBox->FontSize = 30.0;
            reBox->FlowDirection = FlowDirection::RightToLeft;

            LOG_OUTPUT(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::RichEditBox>(reBox);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;
            wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Executing test on UI thread");
                auto richeditBoxAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(reBox);
                VERIFY_IS_NOT_NULL(richeditBoxAsAutomationPeer);
            });

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LOG_OUTPUT(L"Get Text pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern), L" TextBoxAutomationIntegrationTests::GetVisibleRangesOnRTLBox : Failed in fetching Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L" TextBoxAutomationIntegrationTests::GetVisibleRangesOnRTLBox : This element doesn't support Text Pattern which is required.");
            VERIFY_IS_NOT_NULL(spUITextPattern.Get());

            VERIFY_SUCCEEDED(spUITextPattern->GetVisibleRanges(spUITextArrangeArray.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());

            VERIFY_SUCCEEDED(spUITextArrangeArray->GetElement(0, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            int moveCount = 0;
            VERIFY_SUCCEEDED(spUIAutomationTextRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Word, 1, &moveCount));
            VERIFY_ARE_EQUAL(moveCount, 1);
            VerifyRangeText(spUIAutomationTextRange, L"World");

        });
    }

    void RichEditBoxAutomationIntegrationTests::VerifyContextMenuIsFocused()
    {
        TestCleanupWrapper cleanup;

        // Calling ShowContextMenu() on the AP causes the leak
        // detection tool to crash.
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        xaml_controls::Button^ dummyButton;
        xaml_controls::AppBarToggleButton^ floatieButton;
        xaml_controls::RichEditBox^ richEditBox;
        xaml_automation_peers::RichEditBoxAutomationPeer^ richEditBoxAP;

        Event loadedEvent;
        auto loadedEventRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        Event dummyButtonGotFocus;
        auto dummyButtonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        Event richEditBoxGotFocus;
        auto richEditBoxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, GotFocus);

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" HorizontalAlignment="Left">
                        <Button x:Name="dummyButton" Content="Dummy Button" />
                        <RichEditBox x:Name="richEditBox" Width="200" />
                    </StackPanel>)"));

            dummyButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName("dummyButton"));
            richEditBox = safe_cast<xaml_controls::RichEditBox^>(rootPanel->FindName("richEditBox"));

            loadedEventRegistration.Attach(rootPanel, [&loadedEvent]() { loadedEvent.Set(); });
            richEditBoxGotFocusRegistration.Attach(richEditBox, [&richEditBoxGotFocus]() { richEditBoxGotFocus.Set(); });
            dummyButtonGotFocusRegistration.Attach(dummyButton, [&dummyButtonGotFocus]() { dummyButtonGotFocus.Set(); });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for Loaded event.");
        loadedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Setting focus to RichEditBox using UIA.");
        RunOnUIThread([&]()
        {
            richEditBoxAP = safe_cast<xaml_automation_peers::RichEditBoxAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(richEditBox));
            richEditBoxAP->SetFocus();
        });

        LOG_OUTPUT(L"Waiting for RichEditBox to get focus.");
        richEditBoxGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Inserting text via keyboard.");
        LOG_OUTPUT(L"Last input device used = Keyboard.");
        Platform::String ^strToType = "Yo.";
        TestServices::KeyboardHelper->PressKeySequence(strToType);
        TestServices::WindowHelper->WaitForIdle();

        xaml_primitives::FlyoutBase^ flyoutBase = nullptr;

        auto contextFlyoutOpenedEvent = std::make_shared<Event>();
        auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);

        auto contextFlyoutClosedEvent = std::make_shared<Event>();
        auto contextFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        RunOnUIThread([&]()
        {
            flyoutBase = richEditBox->ContextFlyout;
            VERIFY_IS_NOT_NULL(flyoutBase);

            contextFlyoutOpenedRegistration.Attach(
                flyoutBase,
                ref new wf::EventHandler<Platform::Object^>(
                    [&](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"ContextFlyout opened.");
                contextFlyoutOpenedEvent->Set();
            }));

            contextFlyoutClosedRegistration.Attach(
                flyoutBase,
                ref new wf::EventHandler<Platform::Object^>(
                    [&](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"ContextFlyout closed.");
                contextFlyoutClosedEvent->Set();
            }));
        });

        LOG_OUTPUT(L"Invoking Context Menu using UIA.");
        RunOnUIThread([&]()
        {
            richEditBoxAP->ShowContextMenu();
        });
        contextFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verifying that focus has moved to the first button of the floatie.");
        RunOnUIThread([&]()
        {
            floatieButton = safe_cast<xaml_controls::AppBarToggleButton^>(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot));
            VERIFY_IS_NOT_NULL(floatieButton);
        });

        LOG_OUTPUT(L"Tapping on the dummy Button to dismiss the floatie.");
        LOG_OUTPUT(L"Last input device used = Touch.");
        RunOnUIThread([&]()
        {
            TestServices::InputHelper->Tap(dummyButton);
        });
        contextFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on the dummy Button to focus it.");
        RunOnUIThread([&]()
        {
            TestServices::InputHelper->Tap(dummyButton);
        });
        dummyButtonGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        richEditBoxGotFocus.Reset();
        dummyButtonGotFocus.Reset();
        contextFlyoutOpenedEvent->Reset();
        contextFlyoutClosedEvent->Reset();

        LOG_OUTPUT(L"Setting focus to RichEditBox again using UIA.");
        RunOnUIThread([&]()
        {
            richEditBoxAP->SetFocus();
        });

        LOG_OUTPUT(L"Waiting for RichEditBox to get focus.");
        richEditBoxGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Invoking Context Menu using UIA.");
        RunOnUIThread([&]()
        {
            richEditBoxAP->ShowContextMenu();
        });
        contextFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verifying that focus has moved to the first button of the floatie.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(floatieButton));
        });

        LOG_OUTPUT(L"Tapping on the dummy Button to dismiss the floatie.");
        RunOnUIThread([&]()
        {
            TestServices::InputHelper->Tap(dummyButton);
        });
        contextFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on the dummy Button to focus it.");
        RunOnUIThread([&]()
        {
            TestServices::InputHelper->Tap(dummyButton);
        });
        dummyButtonGotFocus.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RichEditBox
