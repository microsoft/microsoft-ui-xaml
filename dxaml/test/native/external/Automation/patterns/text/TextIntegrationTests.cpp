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
#include <TestEvent.h>
#include <array>
#include <cmath>
#include <string>
#include <vector>

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
    namespace
    {
        namespace docs = ::Microsoft::UI::Xaml::Documents;

        std::wstring GetRangeText(IUIAutomationTextRange* range)
        {
            VERIFY_IS_NOT_NULL(range);
            AutoBSTR text;
            VERIFY_SUCCEEDED(range->GetText(-1, text.ReleaseAndGetAddressOf()));
            return text ? std::wstring(static_cast<BSTR>(text)) : std::wstring();
        }

        void VerifyRangeText(const wchar_t* expected, IUIAutomationTextRange* range)
        {
            const auto actual = GetRangeText(range);
            LOG_OUTPUT(L"Expected range text \"%s\", actual \"%s\".", expected, actual.c_str());
            VERIFY_IS_TRUE(actual == expected);
        }

        bool StartsWith(const std::wstring& text, const wchar_t* prefix)
        {
            return text.compare(0, wcslen(prefix), prefix) == 0;
        }

        bool Contains(const std::wstring& text, const wchar_t* part)
        {
            return text.find(part) != std::wstring::npos;
        }

        wrl::ComPtr<IUIAutomationTextRange> CloneRange(IUIAutomationTextRange* range)
        {
            wrl::ComPtr<IUIAutomationTextRange> clone;
            VERIFY_SUCCEEDED(range->Clone(&clone));
            VERIFY_IS_NOT_NULL(clone.Get());
            return clone;
        }

        // Returns a degenerate range at the start of the given range.
        wrl::ComPtr<IUIAutomationTextRange> CollapsedAtStart(IUIAutomationTextRange* range)
        {
            auto collapsed = CloneRange(range);
            VERIFY_SUCCEEDED(collapsed->MoveEndpointByRange(TextPatternRangeEndpoint_End, collapsed.Get(), TextPatternRangeEndpoint_Start));
            return collapsed;
        }

        int CompareEndpoints(IUIAutomationTextRange* range, TextPatternRangeEndpoint endpoint, IUIAutomationTextRange* target, TextPatternRangeEndpoint targetEndpoint)
        {
            int result = 0;
            VERIFY_SUCCEEDED(range->CompareEndpoints(endpoint, target, targetEndpoint, &result));
            return result;
        }

        // Must be called on the UIA client thread.
        wrl::ComPtr<IUIAutomationElement> FindElementByName(const wchar_t* name)
        {
            Automation::AutomationClient::UIAElementInfo info = {};
            info.m_Name = name;
            auto clientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(info);
            wrl::ComPtr<IUIAutomationElement> element;
            clientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element.Get());
            return element;
        }

        // Must be called on the UIA client thread.
        wrl::ComPtr<IUIAutomationTextRange> GetDocumentRange(const wchar_t* elementName)
        {
            auto element = FindElementByName(elementName);
            wrl::ComPtr<IUIAutomationTextPattern> textPattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &textPattern));
            VERIFY_IS_NOT_NULL(textPattern.Get());
            wrl::ComPtr<IUIAutomationTextRange> document;
            VERIFY_SUCCEEDED(textPattern->get_DocumentRange(&document));
            VERIFY_IS_NOT_NULL(document.Get());
            return document;
        }

        double GetDoubleAttribute(IUIAutomationTextRange* range, TEXTATTRIBUTEID attributeId, const wchar_t* name)
        {
            AutoVariant value;
            VERIFY_SUCCEEDED(range->GetAttributeValue(attributeId, value.ReleaseAndGetAddressOf()));
            AutoVariant converted;
            VERIFY_SUCCEEDED(VariantChangeType(converted.Storage(), value.Storage(), 0, VT_R8));
            LOG_OUTPUT(L"%s = %f (variant type %d).", name, converted.Storage()->dblVal, static_cast<int>(value.Storage()->vt));
            return converted.Storage()->dblVal;
        }

        int GetIntAttribute(IUIAutomationTextRange* range, TEXTATTRIBUTEID attributeId, const wchar_t* name)
        {
            AutoVariant value;
            VERIFY_SUCCEEDED(range->GetAttributeValue(attributeId, value.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(static_cast<int>(VT_I4), static_cast<int>(value.Storage()->vt));
            LOG_OUTPUT(L"%s = 0x%08x.", name, static_cast<unsigned int>(value.Storage()->lVal));
            return static_cast<int>(value.Storage()->lVal);
        }

        bool GetBoolAttribute(IUIAutomationTextRange* range, TEXTATTRIBUTEID attributeId, const wchar_t* name)
        {
            AutoVariant value;
            VERIFY_SUCCEEDED(range->GetAttributeValue(attributeId, value.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(static_cast<int>(VT_BOOL), static_cast<int>(value.Storage()->vt));
            LOG_OUTPUT(L"%s = %d.", name, value.Storage()->boolVal == VARIANT_TRUE ? 1 : 0);
            return value.Storage()->boolVal == VARIANT_TRUE;
        }

        std::vector<std::array<double, 4>> GetRectangles(IUIAutomationTextRange* range)
        {
            SAFEARRAY* values = nullptr;
            VERIFY_SUCCEEDED(range->GetBoundingRectangles(&values));
            auto destroyValues = wil::scope_exit([&]() { if (values) { SafeArrayDestroy(values); } });

            std::vector<std::array<double, 4>> rectangles;
            if (values != nullptr)
            {
                LONG lower = 0;
                LONG upper = -1;
                VERIFY_SUCCEEDED(SafeArrayGetLBound(values, 1, &lower));
                VERIFY_SUCCEEDED(SafeArrayGetUBound(values, 1, &upper));
                const LONG count = upper - lower + 1;
                VERIFY_ARE_EQUAL(0, static_cast<int>(count % 4));
                for (LONG i = 0; i < count; i += 4)
                {
                    std::array<double, 4> rectangle = {};
                    for (LONG j = 0; j < 4; ++j)
                    {
                        LONG index = lower + i + j;
                        VERIFY_SUCCEEDED(SafeArrayGetElement(values, &index, &rectangle[j]));
                    }
                    LOG_OUTPUT(L"Rectangle: x=%f y=%f width=%f height=%f", rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
                    rectangles.push_back(rectangle);
                }
            }
            return rectangles;
        }

        docs::Run^ MakeRun(const wchar_t* text)
        {
            auto run = ref new docs::Run();
            run->Text = ref new Platform::String(text);
            return run;
        }

        // Builds the color directly; Windows::UI::ColorHelper would load Windows.UI.Xaml.dll into the test process.
        ::Windows::UI::Color MakeColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
        {
            ::Windows::UI::Color color = {};
            color.A = a;
            color.R = r;
            color.G = g;
            color.B = b;
            return color;
        }

        // Moves the start of a copy of 'source' forward by far more units than remain and verifies the contract for moving
        // an endpoint past the other one and past the end of the document: the range becomes degenerate, lies after the
        // original range, and only whitespace (no document text) remains between it and the end of the document.
        void VerifyStartClampedPastDocumentEnd(IUIAutomationTextRange* document, IUIAutomationTextRange* source, TextUnit unit, const wchar_t* unitName)
        {
            LOG_OUTPUT(L"Moving the start by 100 %s units clamps the range to the end of the document.", unitName);
            auto range = CloneRange(source);
            int moved = 0;
            VERIFY_SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, unit, 100, &moved));
            LOG_OUTPUT(L"MoveEndpointByUnit(Start, %s, 100) moved %d units.", unitName, moved);
            VERIFY_IS_GREATER_THAN(moved, 0);
            VERIFY_IS_LESS_THAN(moved, 100);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(range.Get(), TextPatternRangeEndpoint_Start, range.Get(), TextPatternRangeEndpoint_End));
            VerifyRangeText(L"", range.Get());
            VERIFY_IS_GREATER_THAN(CompareEndpoints(range.Get(), TextPatternRangeEndpoint_Start, source, TextPatternRangeEndpoint_End), 0);
            VERIFY_IS_LESS_THAN_OR_EQUAL(CompareEndpoints(range.Get(), TextPatternRangeEndpoint_End, document, TextPatternRangeEndpoint_End), 0);

            auto rest = CloneRange(range.Get());
            VERIFY_SUCCEEDED(rest->MoveEndpointByRange(TextPatternRangeEndpoint_End, document, TextPatternRangeEndpoint_End));
            const auto restText = GetRangeText(rest.Get());
            LOG_OUTPUT(L"Text between the clamped range and the document end has %d characters.", static_cast<int>(restText.size()));
            for (const wchar_t ch : restText)
            {
                VERIFY_IS_TRUE(!!iswspace(ch));
            }
        }
    }

    void TextIntegrationTests::VerifyTextRangeCompareAndCompareEndpoints()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            auto first = ref new xaml_controls::TextBlock();
            first->Text = L"Hello World";
            xaml_automation::AutomationProperties::SetName(first, L"CompareTextBlock");
            auto second = ref new xaml_controls::TextBlock();
            second->Text = L"Other text";
            xaml_automation::AutomationProperties::SetName(second, L"OtherTextBlock");
            root->Children->Append(first);
            root->Children->Append(second);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"CompareTextBlock");
            auto clone = CloneRange(document.Get());

            LOG_OUTPUT(L"A clone compares equal to the original and has the same endpoints.");
            BOOL areSame = FALSE;
            VERIFY_SUCCEEDED(document->Compare(clone.Get(), &areSame));
            VERIFY_IS_TRUE(!!areSame);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(document.Get(), TextPatternRangeEndpoint_Start, clone.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(document.Get(), TextPatternRangeEndpoint_End, clone.Get(), TextPatternRangeEndpoint_End));
            const int documentSpan = CompareEndpoints(document.Get(), TextPatternRangeEndpoint_End, document.Get(), TextPatternRangeEndpoint_Start);
            LOG_OUTPUT(L"Document span = %d.", documentSpan);
            VERIFY_IS_GREATER_THAN(documentSpan, 0);

            LOG_OUTPUT(L"Moving the clone's start forward makes Compare false and the endpoint differences signed, antisymmetric and consistent with the span.");
            int moved = 0;
            VERIFY_SUCCEEDED(clone->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 6, &moved));
            VERIFY_ARE_EQUAL(6, moved);
            const auto cloneText = GetRangeText(clone.Get());
            VERIFY_IS_TRUE(StartsWith(cloneText, L"World"));
            VERIFY_IS_FALSE(Contains(cloneText, L"Hello"));
            VERIFY_SUCCEEDED(document->Compare(clone.Get(), &areSame));
            VERIFY_IS_FALSE(!!areSame);
            // Endpoint differences are measured in text positions, which are not required to equal character counts.
            const int startDelta = CompareEndpoints(document.Get(), TextPatternRangeEndpoint_Start, clone.Get(), TextPatternRangeEndpoint_Start);
            LOG_OUTPUT(L"Document start minus clone start = %d.", startDelta);
            VERIFY_IS_LESS_THAN(startDelta, 0);
            VERIFY_IS_LESS_THAN(-startDelta, documentSpan);
            VERIFY_ARE_EQUAL(-startDelta, CompareEndpoints(clone.Get(), TextPatternRangeEndpoint_Start, document.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(clone.Get(), TextPatternRangeEndpoint_End, document.Get(), TextPatternRangeEndpoint_End));
            VERIFY_ARE_EQUAL(documentSpan + startDelta, CompareEndpoints(clone.Get(), TextPatternRangeEndpoint_End, clone.Get(), TextPatternRangeEndpoint_Start));

            LOG_OUTPUT(L"Ranges from another TextBlock cannot be compared.");
            auto other = GetDocumentRange(L"OtherTextBlock");
            const HRESULT compareResult = document->Compare(other.Get(), &areSame);
            LOG_OUTPUT(L"Compare with another TextBlock's range returned 0x%08x.", compareResult);
            VERIFY_IS_TRUE(FAILED(compareResult));
            int compareEndpointsValue = 0;
            const HRESULT compareEndpointsResult = document->CompareEndpoints(TextPatternRangeEndpoint_Start, other.Get(), TextPatternRangeEndpoint_Start, &compareEndpointsValue);
            LOG_OUTPUT(L"CompareEndpoints with another TextBlock's range returned 0x%08x.", compareEndpointsResult);
            VERIFY_IS_TRUE(FAILED(compareEndpointsResult));
        });
    }

    void TextIntegrationTests::VerifyParagraphPageAndDocumentUnits()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            auto richTextBlock = ref new xaml_controls::RichTextBlock();
            xaml_automation::AutomationProperties::SetName(richTextBlock, L"ParagraphRichTextBlock");
            for (auto text : { L"First para", L"Second para", L"Third para" })
            {
                auto paragraph = ref new docs::Paragraph();
                paragraph->Inlines->Append(MakeRun(text));
                richTextBlock->Blocks->Append(paragraph);
            }
            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = L"Single block";
            xaml_automation::AutomationProperties::SetName(textBlock, L"ParagraphTextBlock");
            root->Children->Append(richTextBlock);
            root->Children->Append(textBlock);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"ParagraphRichTextBlock");
            const auto documentText = GetRangeText(document.Get());
            VERIFY_IS_TRUE(Contains(documentText, L"First para"));
            VERIFY_IS_TRUE(Contains(documentText, L"Second para"));
            VERIFY_IS_TRUE(Contains(documentText, L"Third para"));

            LOG_OUTPUT(L"ExpandToEnclosingUnit(Paragraph) from the start selects only the first paragraph.");
            auto range = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(range->ExpandToEnclosingUnit(TextUnit_Paragraph));
            auto text = GetRangeText(range.Get());
            VERIFY_IS_TRUE(StartsWith(text, L"First para"));
            VERIFY_IS_FALSE(Contains(text, L"Second"));

            VerifyStartClampedPastDocumentEnd(document.Get(), range.Get(), TextUnit_Paragraph, L"Paragraph");

            LOG_OUTPUT(L"Move by one paragraph selects the second paragraph.");
            int moved = 0;
            VERIFY_SUCCEEDED(range->Move(TextUnit_Paragraph, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            text = GetRangeText(range.Get());
            VERIFY_IS_TRUE(StartsWith(text, L"Second para"));
            VERIFY_IS_FALSE(Contains(text, L"First"));
            VERIFY_IS_FALSE(Contains(text, L"Third"));

            LOG_OUTPUT(L"Moving past the last paragraph stops on the last paragraph.");
            VERIFY_SUCCEEDED(range->Move(TextUnit_Paragraph, 5, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            text = GetRangeText(range.Get());
            VERIFY_IS_TRUE(StartsWith(text, L"Third para"));

            LOG_OUTPUT(L"Moving back two paragraphs returns to the first paragraph.");
            VERIFY_SUCCEEDED(range->Move(TextUnit_Paragraph, -2, &moved));
            VERIFY_ARE_EQUAL(-2, moved);
            text = GetRangeText(range.Get());
            VERIFY_IS_TRUE(StartsWith(text, L"First para"));
            VERIFY_IS_FALSE(Contains(text, L"Second"));

            LOG_OUTPUT(L"Moving the end by one paragraph extends the range over the second paragraph.");
            VERIFY_SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Paragraph, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            text = GetRangeText(range.Get());
            VERIFY_IS_TRUE(StartsWith(text, L"First para"));
            VERIFY_IS_TRUE(Contains(text, L"Second para"));
            VERIFY_IS_FALSE(Contains(text, L"Third"));

            LOG_OUTPUT(L"Page and Document units enclose the whole document.");
            auto page = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(page->ExpandToEnclosingUnit(TextUnit_Page));
            VERIFY_IS_TRUE(GetRangeText(page.Get()) == documentText);
            auto whole = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(whole->ExpandToEnclosingUnit(TextUnit_Document));
            VERIFY_IS_TRUE(GetRangeText(whole.Get()) == documentText);

            LOG_OUTPUT(L"Moving endpoints by Document units moves them to the document start or end.");
            auto endpoints = CloneRange(document.Get());
            VERIFY_SUCCEEDED(endpoints->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Document, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endpoints.Get(), TextPatternRangeEndpoint_Start, document.Get(), TextPatternRangeEndpoint_End));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endpoints.Get(), TextPatternRangeEndpoint_End, document.Get(), TextPatternRangeEndpoint_End));
            VERIFY_SUCCEEDED(endpoints->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Document, -1, &moved));
            VERIFY_ARE_EQUAL(-1, moved);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endpoints.Get(), TextPatternRangeEndpoint_Start, document.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_SUCCEEDED(endpoints->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Document, -1, &moved));
            VERIFY_ARE_EQUAL(-1, moved);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endpoints.Get(), TextPatternRangeEndpoint_End, document.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_SUCCEEDED(endpoints->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Document, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endpoints.Get(), TextPatternRangeEndpoint_End, document.Get(), TextPatternRangeEndpoint_End));

            LOG_OUTPUT(L"A TextBlock has no paragraphs, so the Paragraph unit is the whole document.");
            auto textBlockDocument = GetDocumentRange(L"ParagraphTextBlock");
            auto textBlockRange = CollapsedAtStart(textBlockDocument.Get());
            VERIFY_SUCCEEDED(textBlockRange->ExpandToEnclosingUnit(TextUnit_Paragraph));
            BOOL areSame = FALSE;
            VERIFY_SUCCEEDED(textBlockRange->Compare(textBlockDocument.Get(), &areSame));
            VERIFY_IS_TRUE(!!areSame);
            auto textBlockEnd = CloneRange(textBlockDocument.Get());
            VERIFY_SUCCEEDED(textBlockEnd->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Paragraph, -1, &moved));
            VERIFY_ARE_EQUAL(-1, moved);
            VERIFY_ARE_EQUAL(0, CompareEndpoints(textBlockEnd.Get(), TextPatternRangeEndpoint_End, textBlockDocument.Get(), TextPatternRangeEndpoint_Start));
        });
    }

    void TextIntegrationTests::VerifyFormatUnitBoundaries()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();

            auto textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, L"FormatTextBlock");
            textBlock->Inlines->Append(MakeRun(L"Plain "));
            auto bold = ref new docs::Bold();
            bold->Inlines->Append(MakeRun(L"Bold"));
            textBlock->Inlines->Append(bold);
            textBlock->Inlines->Append(MakeRun(L" mid "));
            auto italic = ref new docs::Italic();
            auto italicRun = MakeRun(L"Ital");
            italicRun->Foreground = ref new xaml_media::SolidColorBrush(MakeColor(0xFF, 0xC0, 0x10, 0x20));
            italic->Inlines->Append(italicRun);
            textBlock->Inlines->Append(italic);
            textBlock->Inlines->Append(ref new docs::LineBreak());
            textBlock->Inlines->Append(MakeRun(L"Tail"));
            root->Children->Append(textBlock);

            auto richTextBlock = ref new xaml_controls::RichTextBlock();
            xaml_automation::AutomationProperties::SetName(richTextBlock, L"FormatRichTextBlock");
            auto paragraph = ref new docs::Paragraph();
            paragraph->Inlines->Append(MakeRun(L"Before"));
            auto richBold = ref new docs::Bold();
            richBold->Inlines->Append(MakeRun(L"After"));
            paragraph->Inlines->Append(richBold);
            richTextBlock->Blocks->Append(paragraph);
            root->Children->Append(richTextBlock);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"FormatTextBlock");

            LOG_OUTPUT(L"Each run with different formatting is a separate Format unit.");
            auto range = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(range->ExpandToEnclosingUnit(TextUnit_Format));
            VerifyRangeText(L"Plain ", range.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(range->Move(TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"Bold", range.Get());
            VERIFY_SUCCEEDED(range->Move(TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L" mid ", range.Get());
            VERIFY_SUCCEEDED(range->Move(TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"Ital", range.Get());

            LOG_OUTPUT(L"The next Format unit after the line break contains the last run.");
            auto afterBreak = CloneRange(range.Get());
            VERIFY_SUCCEEDED(afterBreak->Move(TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            const auto afterBreakText = GetRangeText(afterBreak.Get());
            LOG_OUTPUT(L"Format unit after the line break: \"%s\".", afterBreakText.c_str());
            VERIFY_IS_TRUE(Contains(afterBreakText, L"Tail"));
            VERIFY_IS_FALSE(Contains(afterBreakText, L"Ital"));

            LOG_OUTPUT(L"Moving back three Format units returns to the first run.");
            VERIFY_SUCCEEDED(range->Move(TextUnit_Format, -3, &moved));
            VERIFY_ARE_EQUAL(-3, moved);
            VerifyRangeText(L"Plain ", range.Get());

            LOG_OUTPUT(L"Moving the end by one Format unit extends the range over the bold run.");
            VERIFY_SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"Plain Bold", range.Get());

            LOG_OUTPUT(L"A position inside the italic run expands to the whole italic run.");
            auto inner = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(inner->Move(TextUnit_Character, 16, &moved));
            VERIFY_ARE_EQUAL(16, moved);
            VerifyRangeText(L"t", inner.Get());
            VERIFY_SUCCEEDED(inner->ExpandToEnclosingUnit(TextUnit_Format));
            VerifyRangeText(L"Ital", inner.Get());

            LOG_OUTPUT(L"In a RichTextBlock paragraph, a bold span ends the first Format unit.");
            auto richDocument = GetDocumentRange(L"FormatRichTextBlock");
            auto before = CollapsedAtStart(richDocument.Get());
            VERIFY_SUCCEEDED(before->ExpandToEnclosingUnit(TextUnit_Format));
            const auto beforeText = GetRangeText(before.Get());
            LOG_OUTPUT(L"First Format unit of the RichTextBlock: \"%s\".", beforeText.c_str());
            VerifyRangeText(L"Before", before.Get());
            VERIFY_SUCCEEDED(before->Move(TextUnit_Format, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            const auto afterText = GetRangeText(before.Get());
            LOG_OUTPUT(L"Second Format unit of the RichTextBlock: \"%s\".", afterText.c_str());
            VERIFY_IS_TRUE(StartsWith(afterText, L"After"));
            VERIFY_IS_FALSE(Contains(afterText, L"Before"));
        });
    }

    void TextIntegrationTests::VerifyTextAttributeValuesFromElements()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();

            auto richTextBlock = ref new xaml_controls::RichTextBlock();
            xaml_automation::AutomationProperties::SetName(richTextBlock, L"AttributeRichTextBlock");
            richTextBlock->Margin = xaml::ThicknessHelper::FromLengths(1, 2, 3, 4);
            richTextBlock->TextIndent = 5;
            richTextBlock->HorizontalAlignment = xaml::HorizontalAlignment::Center;
            auto paragraph = ref new docs::Paragraph();
            paragraph->Margin = xaml::ThicknessHelper::FromLengths(10, 20, 30, 40);
            paragraph->TextIndent = 7;
            auto styled = MakeRun(L"Styled");
            styled->FontSize = 24;
            ::Windows::UI::Text::FontWeight weight = {};
            weight.Weight = 700;
            styled->FontWeight = weight;
            styled->Language = L"fr-FR";
            styled->Foreground = ref new xaml_media::SolidColorBrush(MakeColor(0xFF, 0x12, 0x34, 0x56));
            docs::Typography::SetVariants(styled, xaml::FontVariants::Subscript);
            paragraph->Inlines->Append(styled);
            richTextBlock->Blocks->Append(paragraph);
            root->Children->Append(richTextBlock);

            auto mixedTextBlock = ref new xaml_controls::RichTextBlock();
            xaml_automation::AutomationProperties::SetName(mixedTextBlock, L"MixedRichTextBlock");
            auto mixedParagraph = ref new docs::Paragraph();
            auto smallRun = MakeRun(L"ab");
            smallRun->FontSize = 10;
            auto largeRun = MakeRun(L"cd");
            largeRun->FontSize = 30;
            mixedParagraph->Inlines->Append(smallRun);
            mixedParagraph->Inlines->Append(largeRun);
            mixedTextBlock->Blocks->Append(mixedParagraph);
            root->Children->Append(mixedTextBlock);

            auto textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, L"AttributeTextBlock");
            textBlock->Text = L"Plain";
            textBlock->Margin = xaml::ThicknessHelper::FromLengths(5, 6, 7, 8);
            root->Children->Append(textBlock);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LOG_OUTPUT(L"Attributes of a range inside a run inside a paragraph of a RichTextBlock.");
            auto document = GetDocumentRange(L"AttributeRichTextBlock");
            auto range = CollapsedAtStart(document.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 4, &moved));
            VERIFY_ARE_EQUAL(4, moved);
            VERIFY_SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"tyl", range.Get());

            VERIFY_ARE_EQUAL(24.0, GetDoubleAttribute(range.Get(), UIA_FontSizeAttributeId, L"FontSize"));
            VERIFY_ARE_EQUAL(700, GetIntAttribute(range.Get(), UIA_FontWeightAttributeId, L"FontWeight"));
            VERIFY_ARE_EQUAL(0x040C, GetIntAttribute(range.Get(), UIA_CultureAttributeId, L"Culture"));
            VERIFY_ARE_EQUAL(0x563412, GetIntAttribute(range.Get(), UIA_ForegroundColorAttributeId, L"ForegroundColor"));
            VERIFY_ARE_EQUAL(0x563412, GetIntAttribute(range.Get(), UIA_UnderlineColorAttributeId, L"UnderlineColor"));
            VERIFY_ARE_EQUAL(0x563412, GetIntAttribute(range.Get(), UIA_StrikethroughColorAttributeId, L"StrikethroughColor"));
            VERIFY_ARE_EQUAL(static_cast<int>(HorizontalTextAlignment_Centered), GetIntAttribute(range.Get(), UIA_HorizontalTextAlignmentAttributeId, L"HorizontalTextAlignment"));
            VERIFY_ARE_EQUAL(12.0, GetDoubleAttribute(range.Get(), UIA_IndentationFirstLineAttributeId, L"IndentationFirstLine"));
            VERIFY_ARE_EQUAL(11.0, GetDoubleAttribute(range.Get(), UIA_MarginLeadingAttributeId, L"MarginLeading"));
            VERIFY_ARE_EQUAL(22.0, GetDoubleAttribute(range.Get(), UIA_MarginTopAttributeId, L"MarginTop"));
            VERIFY_ARE_EQUAL(33.0, GetDoubleAttribute(range.Get(), UIA_MarginTrailingAttributeId, L"MarginTrailing"));
            VERIFY_ARE_EQUAL(44.0, GetDoubleAttribute(range.Get(), UIA_MarginBottomAttributeId, L"MarginBottom"));
            VERIFY_IS_TRUE(GetBoolAttribute(range.Get(), UIA_IsSubscriptAttributeId, L"IsSubscript"));
            VERIFY_IS_FALSE(GetBoolAttribute(range.Get(), UIA_IsSuperscriptAttributeId, L"IsSuperscript"));
            VERIFY_IS_FALSE(GetBoolAttribute(range.Get(), UIA_IsHiddenAttributeId, L"IsHidden"));

            LOG_OUTPUT(L"A range over runs with different font sizes reports the mixed attribute value.");
            auto mixedDocument = GetDocumentRange(L"MixedRichTextBlock");
            auto mixed = CollapsedAtStart(mixedDocument.Get());
            VERIFY_SUCCEEDED(mixed->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 3, &moved));
            VERIFY_ARE_EQUAL(3, moved);
            VERIFY_SUCCEEDED(mixed->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"bc", mixed.Get());
            AutoVariant mixedValue;
            VERIFY_SUCCEEDED(mixed->GetAttributeValue(UIA_FontSizeAttributeId, mixedValue.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(static_cast<int>(VT_UNKNOWN), static_cast<int>(mixedValue.Storage()->vt));
            AutomationClient::AutomationClientManager clientManager;
            wrl::ComPtr<IUIAutomation> automation;
            clientManager.GetAutomation(&automation);
            wrl::ComPtr<IUnknown> mixedAttributeValue;
            VERIFY_SUCCEEDED(automation->get_ReservedMixedAttributeValue(&mixedAttributeValue));
            VERIFY_IS_TRUE(mixedValue.Storage()->punkVal == mixedAttributeValue.Get());
            auto singleSize = CollapsedAtStart(mixedDocument.Get());
            VERIFY_SUCCEEDED(singleSize->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 2, &moved));
            VERIFY_ARE_EQUAL(2, moved);
            VERIFY_SUCCEEDED(singleSize->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"b", singleSize.Get());
            VERIFY_ARE_EQUAL(10.0, GetDoubleAttribute(singleSize.Get(), UIA_FontSizeAttributeId, L"FontSize of the first run"));

            LOG_OUTPUT(L"A TextBlock has no paragraph, so margins come from the TextBlock alone and there is no first-line indent.");
            auto textBlockDocument = GetDocumentRange(L"AttributeTextBlock");
            auto textBlockRange = CollapsedAtStart(textBlockDocument.Get());
            VERIFY_SUCCEEDED(textBlockRange->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 4, &moved));
            VERIFY_ARE_EQUAL(4, moved);
            VERIFY_SUCCEEDED(textBlockRange->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 1, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VerifyRangeText(L"lai", textBlockRange.Get());
            VERIFY_ARE_EQUAL(5.0, GetDoubleAttribute(textBlockRange.Get(), UIA_MarginLeadingAttributeId, L"MarginLeading"));
            VERIFY_ARE_EQUAL(6.0, GetDoubleAttribute(textBlockRange.Get(), UIA_MarginTopAttributeId, L"MarginTop"));
            VERIFY_ARE_EQUAL(7.0, GetDoubleAttribute(textBlockRange.Get(), UIA_MarginTrailingAttributeId, L"MarginTrailing"));
            VERIFY_ARE_EQUAL(8.0, GetDoubleAttribute(textBlockRange.Get(), UIA_MarginBottomAttributeId, L"MarginBottom"));
            VERIFY_ARE_EQUAL(0.0, GetDoubleAttribute(textBlockRange.Get(), UIA_IndentationFirstLineAttributeId, L"IndentationFirstLine"));
            VERIFY_IS_FALSE(GetBoolAttribute(textBlockRange.Get(), UIA_IsSubscriptAttributeId, L"IsSubscript"));
        });
    }

    void TextIntegrationTests::VerifyGetBoundingRectangles()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, L"WrappedTextBlock");
            textBlock->Text = L"alpha beta gamma delta epsilon zeta";
            textBlock->FontSize = 20;
            textBlock->Width = 150;
            textBlock->TextWrapping = xaml::TextWrapping::Wrap;
            textBlock->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            textBlock->VerticalAlignment = xaml::VerticalAlignment::Top;
            TestServices::WindowHelper->WindowContent = textBlock;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindElementByName(L"WrappedTextBlock");
            RECT bounds = {};
            VERIFY_SUCCEEDED(element->get_CurrentBoundingRectangle(&bounds));
            LOG_OUTPUT(L"TextBlock bounds: left %ld, top %ld, right %ld, bottom %ld.", bounds.left, bounds.top, bounds.right, bounds.bottom);

            wrl::ComPtr<IUIAutomationTextPattern> textPattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &textPattern));
            VERIFY_IS_NOT_NULL(textPattern.Get());
            wrl::ComPtr<IUIAutomationTextRange> document;
            VERIFY_SUCCEEDED(textPattern->get_DocumentRange(&document));

            LOG_OUTPUT(L"The wrapped document range has one rectangle per line, inside the TextBlock, from top to bottom.");
            const auto lines = GetRectangles(document.Get());
            VERIFY_IS_GREATER_THAN_OR_EQUAL(static_cast<unsigned int>(lines.size()), 2u);
            const double tolerance = 2.0;
            for (const auto& line : lines)
            {
                VERIFY_IS_GREATER_THAN(line[2], 0.0);
                VERIFY_IS_GREATER_THAN(line[3], 0.0);
                VERIFY_IS_GREATER_THAN_OR_EQUAL(line[0], bounds.left - tolerance);
                VERIFY_IS_GREATER_THAN_OR_EQUAL(line[1], bounds.top - tolerance);
                VERIFY_IS_LESS_THAN_OR_EQUAL(line[0] + line[2], bounds.right + tolerance);
                VERIFY_IS_LESS_THAN_OR_EQUAL(line[1] + line[3], bounds.bottom + tolerance);
            }
            VERIFY_IS_GREATER_THAN(lines[1][1], lines[0][1]);

            LOG_OUTPUT(L"The first word has a single rectangle that starts with the first line and is narrower than it.");
            auto word = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(word->ExpandToEnclosingUnit(TextUnit_Word));
            VerifyRangeText(L"alpha ", word.Get());
            const auto wordRectangles = GetRectangles(word.Get());
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(wordRectangles.size()));
            VERIFY_IS_LESS_THAN(wordRectangles[0][2], lines[0][2]);
            VERIFY_IS_LESS_THAN(std::abs(wordRectangles[0][0] - lines[0][0]), 1.0);
            VERIFY_IS_LESS_THAN(std::abs(wordRectangles[0][1] - lines[0][1]), 1.0);
        });
    }

    void TextIntegrationTests::VerifyTextRangeScrollIntoView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                scrollViewer = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            scrollViewer = ref new xaml_controls::ScrollViewer();
            scrollViewer->Height = 100;
            scrollViewer->Width = 300;
            scrollViewer->VerticalAlignment = xaml::VerticalAlignment::Top;
            auto textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, L"ScrollTextBlock");
            for (int i = 0; i < 40; ++i)
            {
                if (i > 0)
                {
                    textBlock->Inlines->Append(ref new docs::LineBreak());
                }
                textBlock->Inlines->Append(MakeRun((L"Line " + std::to_wstring(i)).c_str()));
            }
            scrollViewer->Content = textBlock;

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            TestServices::WindowHelper->WindowContent = scrollViewer;
        });
        TestServices::WindowHelper->WaitForIdle();

        double initialOffset = -1.0;
        RunOnUIThread([&]()
        {
            initialOffset = scrollViewer->VerticalOffset;
        });
        VERIFY_ARE_EQUAL(0.0, initialOffset);

        LOG_OUTPUT(L"ScrollIntoView on the last line scrolls the ScrollViewer down.");
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"ScrollTextBlock");
            auto last = CloneRange(document.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(last->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Document, 1, &moved));
            VERIFY_SUCCEEDED(last->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, -7, &moved));
            VERIFY_ARE_EQUAL(-7, moved);
            VERIFY_IS_TRUE(Contains(GetRangeText(last.Get()), L"Line 39"));
            VERIFY_SUCCEEDED(last->ScrollIntoView(TRUE));
        });
        viewChangedEvent->WaitForDefault();

        double scrolledOffset = 0.0;
        RunOnUIThread([&]()
        {
            scrolledOffset = scrollViewer->VerticalOffset;
        });
        LOG_OUTPUT(L"Vertical offset after scrolling to the last line: %f.", scrolledOffset);
        VERIFY_IS_GREATER_THAN(scrolledOffset, 0.0);

        LOG_OUTPUT(L"ScrollIntoView on the first line scrolls the ScrollViewer back up.");
        viewChangedEvent->Reset();
        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"ScrollTextBlock");
            auto first = CollapsedAtStart(document.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(first->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 6, &moved));
            VerifyRangeText(L"Line 0", first.Get());
            VERIFY_SUCCEEDED(first->ScrollIntoView(TRUE));
        });
        viewChangedEvent->WaitForDefault();

        double finalOffset = -1.0;
        RunOnUIThread([&]()
        {
            finalOffset = scrollViewer->VerticalOffset;
        });
        LOG_OUTPUT(L"Vertical offset after scrolling to the first line: %f.", finalOffset);
        VERIFY_IS_LESS_THAN(finalOffset, scrolledOffset);
    }

    void TextIntegrationTests::VerifyTextRangeSelectAndSelectionModes()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::TextBlock^ selectable = nullptr;
        xaml_controls::TextBlock^ fixedText = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                selectable = nullptr;
                fixedText = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            selectable = ref new xaml_controls::TextBlock();
            selectable->Text = L"Hello World";
            selectable->IsTextSelectionEnabled = true;
            xaml_automation::AutomationProperties::SetName(selectable, L"SelectableTextBlock");
            fixedText = ref new xaml_controls::TextBlock();
            fixedText->Text = L"Fixed text";
            fixedText->IsTextSelectionEnabled = false;
            xaml_automation::AutomationProperties::SetName(fixedText, L"FixedTextBlock");
            root->Children->Append(selectable);
            root->Children->Append(fixedText);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LOG_OUTPUT(L"Select on a selectable TextBlock selects exactly the range.");
            auto document = GetDocumentRange(L"SelectableTextBlock");
            auto world = CloneRange(document.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(world->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 6, &moved));
            VERIFY_ARE_EQUAL(6, moved);
            auto worldEnd = CollapsedAtStart(world.Get());
            VERIFY_SUCCEEDED(worldEnd->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 5, &moved));
            VERIFY_ARE_EQUAL(5, moved);
            VerifyRangeText(L"World", worldEnd.Get());
            VERIFY_SUCCEEDED(worldEnd->Select());

            LOG_OUTPUT(L"TextBlock ranges do not support multiple selection.");
            const HRESULT addResult = worldEnd->AddToSelection();
            LOG_OUTPUT(L"AddToSelection returned 0x%08x.", addResult);
            VERIFY_IS_TRUE(FAILED(addResult));
            const HRESULT removeResult = worldEnd->RemoveFromSelection();
            LOG_OUTPUT(L"RemoveFromSelection returned 0x%08x.", removeResult);
            VERIFY_IS_TRUE(FAILED(removeResult));

            LOG_OUTPUT(L"Select on a TextBlock without text selection fails.");
            auto fixedDocument = GetDocumentRange(L"FixedTextBlock");
            const HRESULT fixedSelectResult = fixedDocument->Select();
            LOG_OUTPUT(L"Select on a non-selectable TextBlock returned 0x%08x.", fixedSelectResult);
            VERIFY_IS_TRUE(FAILED(fixedSelectResult));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Selected text: \"%s\".", selectable->SelectedText ? selectable->SelectedText->Data() : L"<null>");
            VERIFY_ARE_EQUAL(0, Platform::String::CompareOrdinal(L"World", selectable->SelectedText));
            VERIFY_IS_TRUE(fixedText->SelectedText == nullptr || fixedText->SelectedText->IsEmpty());
        });
    }

    void TextIntegrationTests::VerifyMoveEndpointByRangeAndClamping()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = L"Hello World";
            xaml_automation::AutomationProperties::SetName(textBlock, L"MoveTextBlock");
            auto other = ref new xaml_controls::TextBlock();
            other->Text = L"Other text";
            xaml_automation::AutomationProperties::SetName(other, L"OtherMoveTextBlock");
            root->Children->Append(textBlock);
            root->Children->Append(other);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto document = GetDocumentRange(L"MoveTextBlock");
            int moved = 0;

            auto hello = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(hello->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 5, &moved));
            VERIFY_ARE_EQUAL(5, moved);
            VerifyRangeText(L"Hello", hello.Get());
            auto world = CloneRange(document.Get());
            VERIFY_SUCCEEDED(world->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, 6, &moved));
            VERIFY_ARE_EQUAL(6, moved);

            LOG_OUTPUT(L"Moving the start past the end of the document clamps both endpoints to the end of the document.");
            VerifyStartClampedPastDocumentEnd(document.Get(), hello.Get(), TextUnit_Character, L"Character");
            VerifyStartClampedPastDocumentEnd(document.Get(), hello.Get(), TextUnit_Word, L"Word");
            VerifyStartClampedPastDocumentEnd(document.Get(), hello.Get(), TextUnit_Line, L"Line");

            LOG_OUTPUT(L"Moving the start past the end makes the range degenerate at the new start.");
            auto startPastEnd = CloneRange(hello.Get());
            VERIFY_SUCCEEDED(startPastEnd->MoveEndpointByRange(TextPatternRangeEndpoint_Start, world.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(startPastEnd.Get(), TextPatternRangeEndpoint_Start, world.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(startPastEnd.Get(), TextPatternRangeEndpoint_End, world.Get(), TextPatternRangeEndpoint_Start));
            VerifyRangeText(L"", startPastEnd.Get());

            LOG_OUTPUT(L"Moving the end before the start makes the range degenerate at the new end.");
            auto endBeforeStart = CloneRange(world.Get());
            VERIFY_SUCCEEDED(endBeforeStart->MoveEndpointByRange(TextPatternRangeEndpoint_End, hello.Get(), TextPatternRangeEndpoint_End));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endBeforeStart.Get(), TextPatternRangeEndpoint_Start, hello.Get(), TextPatternRangeEndpoint_End));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(endBeforeStart.Get(), TextPatternRangeEndpoint_End, hello.Get(), TextPatternRangeEndpoint_End));
            VerifyRangeText(L"", endBeforeStart.Get());

            LOG_OUTPUT(L"An endpoint cannot be moved to a range of another TextBlock.");
            auto other = GetDocumentRange(L"OtherMoveTextBlock");
            const HRESULT otherResult = startPastEnd->MoveEndpointByRange(TextPatternRangeEndpoint_Start, other.Get(), TextPatternRangeEndpoint_Start);
            LOG_OUTPUT(L"MoveEndpointByRange with another TextBlock's range returned 0x%08x.", otherResult);
            VERIFY_IS_TRUE(FAILED(otherResult));

            LOG_OUTPUT(L"Moving by characters stops on the last and first character of the document.");
            auto character = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(character->Move(TextUnit_Character, 100, &moved));
            VERIFY_ARE_EQUAL(10, moved);
            VerifyRangeText(L"d", character.Get());
            LOG_OUTPUT(L"Moving forward from the last character of the document does not move the range.");
            auto lastCharacter = CloneRange(character.Get());
            VERIFY_SUCCEEDED(character->Move(TextUnit_Character, 1, &moved));
            VERIFY_ARE_EQUAL(0, moved);
            VerifyRangeText(L"d", character.Get());
            VERIFY_ARE_EQUAL(0, CompareEndpoints(character.Get(), TextPatternRangeEndpoint_Start, lastCharacter.Get(), TextPatternRangeEndpoint_Start));
            VERIFY_ARE_EQUAL(0, CompareEndpoints(character.Get(), TextPatternRangeEndpoint_End, lastCharacter.Get(), TextPatternRangeEndpoint_End));
            VERIFY_SUCCEEDED(character->Move(TextUnit_Character, -100, &moved));
            VERIFY_ARE_EQUAL(-10, moved);
            VerifyRangeText(L"H", character.Get());

            LOG_OUTPUT(L"Moving by words stops on the last word of the document.");
            auto word = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(word->ExpandToEnclosingUnit(TextUnit_Word));
            VerifyRangeText(L"Hello ", word.Get());
            VERIFY_SUCCEEDED(word->Move(TextUnit_Word, 5, &moved));
            VERIFY_ARE_EQUAL(1, moved);
            VERIFY_IS_TRUE(StartsWith(GetRangeText(word.Get()), L"World"));
        });
    }

    void TextIntegrationTests::VerifyHyperlinkChildrenAndEnclosingElement()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            auto textBlock = ref new xaml_controls::TextBlock();
            xaml_automation::AutomationProperties::SetName(textBlock, L"LinkTextBlock");
            textBlock->Inlines->Append(MakeRun(L"go "));
            auto hyperlink = ref new docs::Hyperlink();
            xaml_automation::AutomationProperties::SetName(hyperlink, L"TheLink");
            hyperlink->Inlines->Append(MakeRun(L"link"));
            textBlock->Inlines->Append(hyperlink);
            textBlock->Inlines->Append(MakeRun(L" now"));
            root->Children->Append(textBlock);

            auto plain = ref new xaml_controls::TextBlock();
            plain->Text = L"plain words";
            xaml_automation::AutomationProperties::SetName(plain, L"NoLinkTextBlock");
            root->Children->Append(plain);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto verifyName = [](IUIAutomationElement* element, const wchar_t* expectedName)
            {
                VERIFY_IS_NOT_NULL(element);
                AutoBSTR name;
                VERIFY_SUCCEEDED(element->get_CurrentName(name.ReleaseAndGetAddressOf()));
                VERIFY_IS_TRUE(static_cast<bool>(name));
                AutoBSTR::VerifyAreEqual(expectedName, name);
            };
            auto childCount = [](IUIAutomationTextRange* range)
            {
                wrl::ComPtr<IUIAutomationElementArray> children;
                VERIFY_SUCCEEDED(range->GetChildren(&children));
                int length = 0;
                if (children)
                {
                    VERIFY_SUCCEEDED(children->get_Length(&length));
                }
                return length;
            };

            LOG_OUTPUT(L"The document range has the Hyperlink as its only child.");
            auto document = GetDocumentRange(L"LinkTextBlock");
            wrl::ComPtr<IUIAutomationElementArray> children;
            VERIFY_SUCCEEDED(document->GetChildren(&children));
            VERIFY_IS_NOT_NULL(children.Get());
            int length = 0;
            VERIFY_SUCCEEDED(children->get_Length(&length));
            VERIFY_ARE_EQUAL(1, length);
            wrl::ComPtr<IUIAutomationElement> child;
            VERIFY_SUCCEEDED(children->GetElement(0, &child));
            verifyName(child.Get(), L"TheLink");
            CONTROLTYPEID controlType = 0;
            VERIFY_SUCCEEDED(child->get_CurrentControlType(&controlType));
            VERIFY_ARE_EQUAL(static_cast<int>(UIA_HyperlinkControlTypeId), static_cast<int>(controlType));

            LOG_OUTPUT(L"A range inside the link is enclosed by the link and has no children.");
            auto inLink = CollapsedAtStart(document.Get());
            int moved = 0;
            VERIFY_SUCCEEDED(inLink->Move(TextUnit_Character, 4, &moved));
            VERIFY_ARE_EQUAL(4, moved);
            VERIFY_SUCCEEDED(inLink->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &moved));
            VerifyRangeText(L"in", inLink.Get());
            wrl::ComPtr<IUIAutomationElement> enclosing;
            VERIFY_SUCCEEDED(inLink->GetEnclosingElement(&enclosing));
            verifyName(enclosing.Get(), L"TheLink");
            VERIFY_ARE_EQUAL(0, childCount(inLink.Get()));

            LOG_OUTPUT(L"A range before the link is enclosed by the TextBlock and has no children.");
            auto beforeLink = CollapsedAtStart(document.Get());
            VERIFY_SUCCEEDED(beforeLink->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 2, &moved));
            VERIFY_ARE_EQUAL(2, moved);
            VerifyRangeText(L"go", beforeLink.Get());
            VERIFY_SUCCEEDED(beforeLink->GetEnclosingElement(enclosing.ReleaseAndGetAddressOf()));
            verifyName(enclosing.Get(), L"LinkTextBlock");
            VERIFY_ARE_EQUAL(0, childCount(beforeLink.Get()));

            LOG_OUTPUT(L"A TextBlock without links has no text range children.");
            auto plainDocument = GetDocumentRange(L"NoLinkTextBlock");
            VERIFY_ARE_EQUAL(0, childCount(plainDocument.Get()));
        });
    }

} } } } } }
