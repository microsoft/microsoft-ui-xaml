// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CaretBrowsingAutomationIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>
#include <AutomationClient\AutomationEventHandler.h>
#include <AutomationClient\AutomationClientInitializer.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <CustomSystemFontCollectionOverride.h>
#include <SafeEventRegistration.h>
#include <UIAutomationCore.h>
#include <WUCRenderingScopeGuard.h>
#include <TreeHelper.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Text;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Automation::Peers;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

#define HOME_KEY L"$d$_home#$u$_home"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        bool CaretBrowsingAutomationIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool CaretBrowsingAutomationIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool CaretBrowsingAutomationIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void VerifyFocusIsOnElementWithName(const wchar_t* name)
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
                wrl::ComPtr<IUIAutomation> spAutomation;
                spAutomationClientManager->GetAutomation(&spAutomation);
                wrl::ComPtr<IUIAutomationElement> spAutomationElement;
                Common::AutoVariant autoVar;
                spAutomation->GetFocusedElement(&spAutomationElement);
                WEX::Common::Throw::IfNull(spAutomationElement.Get());
                LogThrow_IfFailed(spAutomationElement->GetCurrentPropertyValue(UIA_NamePropertyId, autoVar.Storage()));
                VERIFY_ARE_EQUAL(autoVar.Storage()->vt, VT_BSTR);
                VERIFY_IS_TRUE(!wcscmp(name, (autoVar.Storage())->bstrVal));
            });
        }

        void CaretBrowsingAutomationIntegrationTests::ReceivesFocus()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            RichTextBlockOverflow^ richTextBlockOverflow;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='Hello World' FontSize='15' AutomationProperties.Name='mytb'/>"
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15' Width='50' Height='20' AutomationProperties.Name='myrtb' OverflowContentTarget='{Binding ElementName=myOverflow}'>"
                    L"      <Paragraph>this is some sample text</Paragraph>"
                    L"  </RichTextBlock>"
                    L"  <RichTextBlockOverflow x:Name='myOverflow' AutomationProperties.Name='myOverflow'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"myOverflow"));
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            RunOnUIThread([&]()
            {
                textBlock->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            VerifyFocusIsOnElementWithName(L"mytb");

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyFocusIsOnElementWithName(L"myrtb");

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
             VerifyFocusIsOnElementWithName(L"mytb");

            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
            VerifyFocusIsOnElementWithName(L"myrtb");

            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();
            VerifyFocusIsOnElementWithName(L"myOverflow");
}

        void VerifyCurrentSelection(const wchar_t* selection)
        {
            UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
            {
                auto spAutomationClientManager = std::make_shared<AutomationClient::AutomationClientManager>();
                wrl::ComPtr<IUIAutomation> spAutomation;
                spAutomationClientManager->GetAutomation(&spAutomation);
                wrl::ComPtr<IUIAutomationElement> spAutomationElement;
                Common::AutoVariant autoVar;
                spAutomation->GetFocusedElement(&spAutomationElement);
                VERIFY_IS_NOT_NULL(spAutomationElement.Get());
                wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;
                spAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern);
                VERIFY_IS_NOT_NULL(spUITextPattern.Get());
                wrl::ComPtr<IUIAutomationTextRangeArray> spUITextArrangeArray;
                VERIFY_SUCCEEDED(spUITextPattern->GetSelection(spUITextArrangeArray.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUITextArrangeArray.Get());
                wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
                VERIFY_SUCCEEDED(spUITextArrangeArray->GetElement(0, spUIAutomationTextRange.ReleaseAndGetAddressOf()));
                VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
                AutoBSTR textFromTextRange;
                VERIFY_SUCCEEDED(spUIAutomationTextRange->GetText(-1, textFromTextRange.ReleaseAndGetAddressOf()));
                LOG_OUTPUT(L"Verifying: %s", selection);
                LOG_OUTPUT(L"Selected text is: %s", textFromTextRange.Get());
                VERIFY_IS_TRUE(!wcscmp(selection, textFromTextRange));
            });
        }

        void CaretBrowsingAutomationIntegrationTests::FollowsSelection()
        {
            TestCleanupWrapper cleanup;

            TextBlock^ textBlock;
            RichTextBlock^ richTextBlock;
            RichTextBlockOverflow^ richTextBlockOverflow;

            RunOnUIThread([&]()
            {
                auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel Background='Black' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"  <TextBlock x:Name='mytb' IsTextSelectionEnabled='True' Text='ABCDEFGH' FontSize='15' AutomationProperties.Name='mytb'/>"
                    L"  <RichTextBlock x:Name='myrtb' IsTextSelectionEnabled='True' FontSize='15' Width='50' Height='20' AutomationProperties.Name='myrtb' OverflowContentTarget='{Binding ElementName=myOverflow}'>"
                    L"      <Paragraph>IJKLMNOPQRSTUVWXYZ</Paragraph>"
                    L"  </RichTextBlock>"
                    L"  <RichTextBlockOverflow x:Name='myOverflow' AutomationProperties.Name='myOverflow'/>"
                    L"</StackPanel>"));
                textBlock = safe_cast<TextBlock^>(rootPanel->FindName(L"mytb"));
                VERIFY_IS_NOT_NULL(textBlock);
                richTextBlock = safe_cast<RichTextBlock^>(rootPanel->FindName(L"myrtb"));
                VERIFY_IS_NOT_NULL(richTextBlock);
                richTextBlockOverflow = safe_cast<RichTextBlockOverflow^>(rootPanel->FindName(L"myOverflow"));
                VERIFY_IS_NOT_NULL(richTextBlockOverflow);
                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->SetCaretBrowsingModeGlobal(true, false);

            RunOnUIThread([&]()
            {
                textBlock->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Temporary fix for UiaEndpoint synchronization
            AutomationClient::AutomationClientInitializer::TEMP_WaitForDefault();

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"A");

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"AB");

            TestServices::KeyboardHelper->Tab();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"I");

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"IJ");

            TestServices::KeyboardHelper->PressKeySequence(HOME_KEY);
            TestServices::KeyboardHelper->Down();
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"O");

            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_right#$u$_right#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();
            VerifyCurrentSelection(L"OP");
        }


    } }
} } } }
