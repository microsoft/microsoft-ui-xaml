// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextEditIntegrationTests.h"
#include <Patterns\TextEditPatternHandler.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <collection.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>
#include <array>
#include <MockTextPatternObject.h>

#define IFR(x) { HRESULT __hr = (x); if (!SUCCEEDED(__hr)) return __hr; }

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation::Patterns;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace TextEdit {

    bool TextEditProviderTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TextEditProviderTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TextEditProviderTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void TextEditProviderTests::VerifyTextEditTextChangedEvents()
    {
        TestCleanupWrapper cleanup;
        MockUpProviderControl^ testTextEdit = nullptr;
        MockUpProviderControl^ tb1 = nullptr;
        Platform::Object^ obj = nullptr;
        Platform::Object^ result = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestMock";
        uiaInfo.m_AutomationID = L"TestMock";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            testTextEdit = ref new MockUpProviderControl();
            tb1 = ref new MockUpProviderControl();
            xaml_automation::AutomationProperties::SetName(testTextEdit, ref new Platform::String(uiaInfo.m_Name));
            testTextEdit->Name = "TestMock";
            testTextEdit->FontSize = 30.0;
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::Button>(testTextEdit, true /*wrapInGrid*/);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            MockUpProviderControlRange^ mockRange;
            MockUpProviderControlAutomationPeer^ mockAP;
            xaml_automation_peers::AutomationPeer^ mockAPAsAutomationPeer;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextEditPattern> spUITextEditPattern;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;
            wrl::ComPtr<IUIAutomationTextPattern2> spUITextPattern2;

            WEX::Logging::Log::Comment(L"Executing test on UI thread");
            RunOnUIThread([&]()
            {
                mockAPAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(testTextEdit);
                VERIFY_IS_NOT_NULL(mockAPAsAutomationPeer);
            });
            mockAP = static_cast<MockUpProviderControlAutomationPeer^>(mockAPAsAutomationPeer);
            VERIFY_IS_NOT_NULL(mockAP);
            mockRange = mockAP->_getRange();
            VERIFY_IS_NOT_NULL(mockRange);
            mockRange->initialize();
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            auto spEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                wrl::ComPtr<Patterns::TextEditTextChangedEventHandler> spAutomationTextEditTextChangedEventHandler;
                spAutomationTextEditTextChangedEventHandler.Attach(new Patterns::TextEditTextChangedEventHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, TextEditChangeType::TextEditChangeType_AutoCorrect));

                wfc::IVector<Platform::String^>^ vec = ref new Platform::Collections::Vector<Platform::String^>(0);
                vec->Append(ref new Platform::String(L"Test data1")); vec->Append(ref new Platform::String(L"Test data2"));
                wfc::IVectorView<Platform::String^>^ vecview = vec->GetView();

                mockAP->RaiseTextEditTextChangedEvent(Microsoft::UI::Xaml::Automation::AutomationTextEditChangeType::AutoCorrect, vecview);
                spAutomationTextEditTextChangedEventHandler->RemoveEventHandler();
            });

            WEX::Logging::Log::Comment(L"Test TextEdit pattern methods");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextEditPatternId, __uuidof(IUIAutomationTextEditPattern), &spUITextEditPattern),
                L"TextRangeIntegrationTests::VerifyTextEditTextChangedEvents: Failed in fetching TextEdit Pattern.");
            WEX::Common::Throw::IfNull(spUITextEditPattern.Get(), L"TextRangeIntegrationTests::VerifyTextEditTextChangedEvents: This element doesn't support TextEdit Pattern which is required.");

            mockAP->MakeRangeValid(false);

            VERIFY_SUCCEEDED(spUITextEditPattern->GetActiveComposition(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUITextEditPattern->GetConversionTarget(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(spUIAutomationTextRange.Get());

            mockAP->MakeRangeValid(true);

            VERIFY_SUCCEEDED(spUITextEditPattern->GetActiveComposition(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VERIFY_SUCCEEDED(spUITextEditPattern->GetConversionTarget(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            mockAP->MakeRangeValid(false);

            WEX::Logging::Log::Comment(L"Test Text pattern method");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern),
                L"TextRangeIntegrationTests::VerifyTextEditTextChangedEvents: Failed in fetching Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L"TextRangeIntegrationTests::VerifyTextEditTextChangedEvents: This element doesn't support Text Pattern which is required.");

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(spUIAutomationTextRange.Get());

            mockAP->MakeRangeValid(true);

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(spUIAutomationTextRange.ReleaseAndGetAddressOf()));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            WEX::Logging::Log::Comment(L"Test for exposing of Text2 pattern");
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &spUITextPattern2),
                 L"TextRangeIntegrationTests::VerifyTextEditTextChangedEvents: Failed in fetching Text2 Pattern.");
            VERIFY_IS_NOT_NULL(spUITextPattern2.Get());
        });
    }
    void TextEditProviderTests::VerifyTextEditTextChangedEventChangeTypes()
    {
        TestCleanupWrapper cleanup;
        MockUpProviderControl^ testTextEdit = nullptr;
        MockUpProviderControlAutomationPeer^ mockAP = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                mockAP = nullptr;
                testTextEdit = nullptr;
            });
        });

        Automation::AutomationClient::UIAElementInfo uiaInfo = {};
        uiaInfo.m_Name = L"TextEditChangeTypes";

        RunOnUIThread([&]()
        {
            testTextEdit = ref new MockUpProviderControl();
            xaml_automation::AutomationProperties::SetName(testTextEdit, ref new Platform::String(uiaInfo.m_Name));
            TestServices::WindowHelper->WindowContent = TreeHelper::WrapInGrid(testTextEdit);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            mockAP = static_cast<MockUpProviderControlAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(testTextEdit));
            VERIFY_IS_NOT_NULL(mockAP);
        });

        struct ChangeTypeCase
        {
            Microsoft::UI::Xaml::Automation::AutomationTextEditChangeType xamlChangeType;
            TextEditChangeType uiaChangeType;
            const wchar_t* name;
        };
        // AutomationTextEditChangeType::None raises no UIA event; it is checked separately after these cases.
        const ChangeTypeCase cases[] =
        {
            { Microsoft::UI::Xaml::Automation::AutomationTextEditChangeType::Composition, TextEditChangeType_Composition, L"Composition" },
            { Microsoft::UI::Xaml::Automation::AutomationTextEditChangeType::CompositionFinalized, TextEditChangeType_CompositionFinalized, L"CompositionFinalized" },
        };

        for (const auto& changeTypeCase : cases)
        {
            LOG_OUTPUT(L"Raising a TextEditTextChanged event with change type %s.", changeTypeCase.name);

            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
                auto spEvent = std::make_shared<Event>();

                // The handler only listens for this change type and fails if it is called with another one.
                wrl::ComPtr<Patterns::TextEditTextChangedEventHandler> spHandler;
                spHandler.Attach(new Patterns::TextEditTextChangedEventHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, changeTypeCase.uiaChangeType));
                spHandler->AttachEventHandler();
                // Unregister on every exit path, including a failed raise or wait. RemoveEventHandler is idempotent and can
                // throw, so the guard swallows its failure instead of terminating the process while a test failure unwinds.
                auto unregisterHandler = wil::scope_exit([&]()
                {
                    try
                    {
                        spHandler->RemoveEventHandler();
                    }
                    catch (...)
                    {
                        LOG_OUTPUT(L"Removing the TextEditTextChanged handler during cleanup failed.");
                    }
                });

                RunOnUIThread([&]()
                {
                    auto changedData = ref new Platform::Collections::Vector<Platform::String^>();
                    changedData->Append(ref new Platform::String(changeTypeCase.name));
                    mockAP->RaiseTextEditTextChangedEvent(changeTypeCase.xamlChangeType, changedData->GetView());
                });

                spHandler->ConfirmAndUnregister();
            });
        }

        LOG_OUTPUT(L"Raising a TextEditTextChanged event with change type None raises no UIA event.");
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            auto spEvent = std::make_shared<Event>();
            wrl::ComPtr<Patterns::TextEditTextChangedEventHandler> spHandler;
            spHandler.Attach(new Patterns::TextEditTextChangedEventHandler(spAutomationClientManager, spEvent, TreeScope_Subtree, TextEditChangeType_None));
            spHandler->AttachEventHandler();
            auto unregisterHandler = wil::scope_exit([&]()
            {
                try
                {
                    spHandler->RemoveEventHandler();
                }
                catch (...)
                {
                    LOG_OUTPUT(L"Removing the TextEditTextChanged handler during cleanup failed.");
                }
            });

            RunOnUIThread([&]()
            {
                auto changedData = ref new Platform::Collections::Vector<Platform::String^>();
                changedData->Append(L"None");
                mockAP->RaiseTextEditTextChangedEvent(Microsoft::UI::Xaml::Automation::AutomationTextEditChangeType::None, changedData->GetView());
            });

            spHandler->ConfirmNegativeAndUnregister();
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Automation::AutomationPeer
