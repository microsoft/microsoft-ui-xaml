// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextRangeIntegrationTests.h"
#include <AutomationClient\AutomationClientManager.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <collection.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <TreeHelper.h>

#include <UIAutomationClient.h>
#include <UIAutomationCoreApi.h>
#include <UIAutomationHelper.h>
#include <MockTextPatternObject.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation::Patterns;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace TextRange {

    bool TextRangeIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TextRangeIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TextRangeIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    inline void VerifyGetAttributeValueEnum(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, int valueInitial, int valueFinal)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateInt32(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_I4);
        VERIFY_ARE_EQUAL(varData.Storage()->lVal, valueInitial);

        prop = ::Windows::Foundation::PropertyValue::CreateInt32(valueFinal);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_I4);
        VERIFY_ARE_EQUAL(varData.Storage()->lVal, valueFinal);
    }

    inline void VerifyGetAttributeValueBool(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, bool valueInitial, bool valueFinal)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateBoolean(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
        VERIFY_ARE_EQUAL( (varData.Storage()->boolVal != VARIANT_FALSE), valueInitial);

        prop = ::Windows::Foundation::PropertyValue::CreateBoolean(valueFinal);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BOOL);
        VERIFY_ARE_EQUAL((varData.Storage()->boolVal != VARIANT_FALSE), valueFinal);
    }

    inline void VerifyGetAttributeValueDouble(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, double valueInitial, double valueFinal)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateDouble(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_R8);
        VERIFY_ARE_EQUAL(varData.Storage()->dblVal, valueInitial);

        prop = ::Windows::Foundation::PropertyValue::CreateDouble(valueFinal);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_R8);
        VERIFY_ARE_EQUAL(varData.Storage()->dblVal, valueFinal);
    }

    inline void VerifyGetAttributeValueString(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, const wchar_t* valueInitial, const wchar_t* valueFinal)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        Platform::Object^ prop = ref new Platform::String(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BSTR);
        VERIFY_ARE_EQUAL(wcscmp(varData.Storage()->bstrVal, valueInitial), 0);

        prop = ref new Platform::String(valueFinal);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_BSTR);
        VERIFY_ARE_EQUAL(wcscmp(varData.Storage()->bstrVal, valueFinal), 0);
    }

    inline void VerifyGetAttributeValueDoubleArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        wfc::IVector<double>^ vec = ref new Platform::Collections::Vector<double>(0);
        vec->Append(5); vec->Append(4); vec->Append(3); vec->Append(2); vec->Append(1);
        wfc::IVectorView<double>^ vecview = vec->GetView();
        Platform::Object^ prop = vecview;
        DOUBLE* rgData = nullptr;
        LONG lowerBound, upperBound;  // get array bounds
        testTextRange->setAttribute(attributeid, prop);

        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_ARRAY | VT_R8);
        VERIFY_SUCCEEDED(SafeArrayGetLBound(varData.Storage()->parray, 1, &lowerBound));
        VERIFY_SUCCEEDED(SafeArrayGetUBound(varData.Storage()->parray, 1, &upperBound));
        VERIFY_ARE_EQUAL(upperBound - lowerBound + 1, (DOUBLE)vecview->Size);
        VERIFY_SUCCEEDED(SafeArrayAccessData(varData.Storage()->parray, (void**)&rgData)); // direct access to SA memory
        VERIFY_ARE_EQUAL(rgData[0], 5);
        VERIFY_SUCCEEDED(SafeArrayUnaccessData(varData.Storage()->parray));

        vec->RemoveAt(0);
        vecview = vec->GetView();
        prop = vecview;
        testTextRange->setAttribute(attributeid, prop);

        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_ARRAY | VT_R8);
        VERIFY_SUCCEEDED(SafeArrayGetLBound(varData.Storage()->parray, 1, &lowerBound));
        VERIFY_SUCCEEDED(SafeArrayGetUBound(varData.Storage()->parray, 1, &upperBound));
        VERIFY_ARE_EQUAL(upperBound - lowerBound + 1, (DOUBLE)vecview->Size);
        VERIFY_SUCCEEDED(SafeArrayAccessData(varData.Storage()->parray, (void**)&rgData)); // direct access to SA memory
        VERIFY_ARE_EQUAL(rgData[0], 4);
        VERIFY_SUCCEEDED(SafeArrayUnaccessData(varData.Storage()->parray));
    }

    inline void VerifyGetAttributeValueIntArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);

        AutoVariant varData;
        wfc::IVector<int>^ vec = ref new Platform::Collections::Vector<int>(0);
        vec->Append(5); vec->Append(4); vec->Append(3); vec->Append(2); vec->Append(1);
        wfc::IVectorView<int>^ vecview = vec->GetView();
        Platform::Object^ prop = vecview;
        int* rgData = nullptr;
        LONG lowerBound, upperBound;  // get array bounds
        testTextRange->setAttribute(attributeid, prop);

        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_ARRAY | VT_I4);
        VERIFY_SUCCEEDED(SafeArrayGetLBound(varData.Storage()->parray, 1, &lowerBound));
        VERIFY_SUCCEEDED(SafeArrayGetUBound(varData.Storage()->parray, 1, &upperBound));
        VERIFY_ARE_EQUAL(upperBound - lowerBound + 1, (DOUBLE)vecview->Size);
        VERIFY_SUCCEEDED(SafeArrayAccessData(varData.Storage()->parray, (void**)&rgData)); // direct access to SA memory
        VERIFY_ARE_EQUAL(rgData[0], 5);
        VERIFY_SUCCEEDED(SafeArrayUnaccessData(varData.Storage()->parray));

        vec->RemoveAt(0);
        vecview = vec->GetView();
        prop = vecview;
        testTextRange->setAttribute(attributeid, prop);

        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_ARRAY | VT_I4);
        VERIFY_SUCCEEDED(SafeArrayGetLBound(varData.Storage()->parray, 1, &lowerBound));
        VERIFY_SUCCEEDED(SafeArrayGetUBound(varData.Storage()->parray, 1, &upperBound));
        VERIFY_ARE_EQUAL(upperBound - lowerBound + 1, (DOUBLE)vecview->Size);
        VERIFY_SUCCEEDED(SafeArrayAccessData(varData.Storage()->parray, (void**)&rgData)); // direct access to SA memory
        VERIFY_ARE_EQUAL(rgData[0], 4);
        VERIFY_SUCCEEDED(SafeArrayUnaccessData(varData.Storage()->parray));
    }

    inline void VerifyGetAttributeValueAPArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, xaml_automation_peers::AutomationPeer^ ap)
    {
        WEX::Logging::Log::Comment(pMsg);
        int arraylength = 0;
        AutoVariant varData;
        AutoBSTR elementName;
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ vec = ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeer^>(0);
        vec->Append(ap);
        wfc::IVectorView<xaml_automation_peers::AutomationPeer^>^ vecview = vec->GetView();
        Platform::Object^ prop = vecview;
        xaml_automation_peers::AutomationPeer^ rgData = nullptr;
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_UNKNOWN); //We won't get VT_ARRAY | VT_UNKNOWN, instead we will get VT_UNKNOWN with IUIAutomationElementArray

        wrl::ComPtr<IUIAutomationElement> spAutomationElement;
        wrl::ComPtr<IUnknown> spUnknown = varData.Storage()->punkVal;
        wrl::ComPtr<IUIAutomationElementArray> spUIAutomationElementArray;
        VERIFY_SUCCEEDED(spUnknown.As(&spUIAutomationElementArray));
        VERIFY_SUCCEEDED(spUIAutomationElementArray->get_Length(&arraylength));
        VERIFY_ARE_EQUAL((int)vecview->Size, arraylength);
        VERIFY_SUCCEEDED(spUIAutomationElementArray->GetElement(0, &spAutomationElement));
        VERIFY_SUCCEEDED(spAutomationElement->get_CurrentClassName(elementName.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(wcscmp(elementName, L"TestMock"), 0);
    }

    inline void VerifyGetAttributeValueIUnknown(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);
        AutoVariant varData;
        Platform::Object^ prop;
        AutoBSTR strRangeText;
        prop = testTextRange;
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->GetAttributeValue(attributeid, varData.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(varData.Storage()->vt, VT_UNKNOWN);
        wrl::ComPtr<IUnknown> spUnknown = varData.Storage()->punkVal;
        wrl::ComPtr<IUIAutomationTextRange> spAutomationTextRange;
        VERIFY_SUCCEEDED(spUnknown.As(&spAutomationTextRange));
        VERIFY_SUCCEEDED(spAutomationTextRange->GetText(100, strRangeText.ReleaseAndGetAddressOf()));
        VERIFY_ARE_EQUAL(wcscmp(strRangeText, L"MockUpProviderControlRange"), 0);
    }

    //
    // Test Cases
    //
    void TextRangeIntegrationTests::VerifyGetAttributeValue()
    {
        TestCleanupWrapper cleanup;
        MockUpProviderControl^ testTextRange = nullptr;
        MockUpProviderControl^ tb1 = nullptr;
        Platform::Object^ obj = nullptr;
        Platform::Object^ result = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestMock";
        uiaInfo.m_AutomationID = L"TestMock";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            testTextRange = ref new MockUpProviderControl();
            tb1 = ref new MockUpProviderControl();
            xaml_automation::AutomationProperties::SetName(testTextRange, ref new Platform::String(uiaInfo.m_Name));
            testTextRange->Name = "TestMock";
            testTextRange->FontSize = 30.0;
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::Button>(testTextRange, true /*wrapInGrid*/);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            MockUpProviderControlRange^ mockRange;
            MockUpProviderControlAutomationPeer^ mockAP;
            xaml_automation_peers::AutomationPeer^ mockAPAsAutomationPeer;
            xaml_automation_peers::AutomationPeer^ mockAPAsAutomationPeer1;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;

            WEX::Logging::Log::Comment(L"Executing test on UI thread");

            RunOnUIThread([&]()
            {
                mockAPAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(testTextRange);
                VERIFY_IS_NOT_NULL(mockAPAsAutomationPeer);
                mockAPAsAutomationPeer1 = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tb1);
                VERIFY_IS_NOT_NULL(mockAPAsAutomationPeer1);
            });
            mockAP = static_cast<MockUpProviderControlAutomationPeer^>(mockAPAsAutomationPeer);
            VERIFY_IS_NOT_NULL(mockAP);
            mockRange = mockAP->_getRange();
            VERIFY_IS_NOT_NULL(mockRange);
            mockRange->initialize();

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            VERIFY_IS_NOT_NULL(spAutomationClientManager.get());
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern),
                 L"TextRangeIntegrationTests::VerifyTextAttrributeIds: Failed in retreiving Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L"TextRangeIntegrationTests::VerifyTextAttrributeIds: This element doesn't support Text Pattern which is required.");

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(&spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());

            VerifyGetAttributeValueEnum(L"---UIA_AnimationStyleAttributeId---", UIA_AnimationStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), AnimationStyle_None, AnimationStyle_BlinkingBackground);
            VerifyGetAttributeValueEnum(L"---UIA_BackgroundColorAttributeId---", UIA_BackgroundColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 255);
            VerifyGetAttributeValueEnum(L"---UIA_BulletStyleAttributeId---", UIA_BulletStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), BulletStyle_None, BulletStyle_DashBullet);
            VerifyGetAttributeValueEnum(L"---UIA_CapStyleAttributeId---", UIA_CapStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), CapStyle_None, CapStyle_AllCap);
            VerifyGetAttributeValueEnum(L"---UIA_CultureAttributeId---", UIA_CultureAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 10);
            VerifyGetAttributeValueString(L"---UIA_FontNameAttributeId---", UIA_FontNameAttributeId, mockRange, spUIAutomationTextRange.Get(), L"Arial", L"Courier");
            VerifyGetAttributeValueDouble(L"---UIA_FontSizeAttributeId---", UIA_FontSizeAttributeId, mockRange, spUIAutomationTextRange.Get(), 16, 24);
            VerifyGetAttributeValueEnum(L"---UIA_FontWeightAttributeId---", UIA_FontWeightAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 50);
            VerifyGetAttributeValueEnum(L"---UIA_ForegroundColorAttributeId---", UIA_ForegroundColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 255);
            VerifyGetAttributeValueEnum(L"---UIA_HorizontalTextAlignmentAttributeId---", UIA_HorizontalTextAlignmentAttributeId, mockRange, spUIAutomationTextRange.Get(), HorizontalTextAlignment_Left, HorizontalTextAlignment_Centered);
            VerifyGetAttributeValueDouble(L"---UIA_IndentationFirstLineAttributeId---", UIA_IndentationFirstLineAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 100);
            VerifyGetAttributeValueDouble(L"---UIA_IndentationLeadingAttributeId---", UIA_IndentationLeadingAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 200);
            VerifyGetAttributeValueDouble(L"---UIA_IndentationTrailingAttributeId---", UIA_IndentationTrailingAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 300);
            VerifyGetAttributeValueBool(L"---UIA_IsHiddenAttributeId---", UIA_IsHiddenAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueBool(L"---UIA_IsItalicAttributeId---", UIA_IsItalicAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueBool(L"---UIA_IsReadOnlyAttributeId---", UIA_IsReadOnlyAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueBool(L"---UIA_IsSubscriptAttributeId---", UIA_IsSubscriptAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueBool(L"---UIA_IsSuperscriptAttributeId---", UIA_IsSuperscriptAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueDouble(L"---UIA_MarginBottomAttributeId---", UIA_MarginBottomAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 400);
            VerifyGetAttributeValueDouble(L"---UIA_MarginLeadingAttributeId---", UIA_MarginLeadingAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 500);
            VerifyGetAttributeValueDouble(L"---UIA_MarginTopAttributeId---", UIA_MarginTopAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 600);
            VerifyGetAttributeValueDouble(L"---UIA_MarginTrailingAttributeId---", UIA_MarginTrailingAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 700);
            VerifyGetAttributeValueEnum(L"---UIA_OutlineStylesAttributeId---", UIA_OutlineStylesAttributeId, mockRange, spUIAutomationTextRange.Get(), OutlineStyles_None, OutlineStyles_Embossed);
            VerifyGetAttributeValueEnum(L"---UIA_OverlineColorAttributeId---", UIA_OverlineColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 120);
            VerifyGetAttributeValueEnum(L"---UIA_OverlineStyleAttributeId---", UIA_OverlineStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_None, TextDecorationLineStyle_DashDot);
            VerifyGetAttributeValueEnum(L"---UIA_StrikethroughColorAttributeId---", UIA_StrikethroughColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 220);
            VerifyGetAttributeValueEnum(L"---UIA_StrikethroughStyleAttributeId---", UIA_StrikethroughStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_None, TextDecorationLineStyle_Double);
            VerifyGetAttributeValueDoubleArray(L"---UIA_TabsAttributeId---", UIA_TabsAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyGetAttributeValueEnum(L"---UIA_TextFlowDirectionsAttributeId---", UIA_TextFlowDirectionsAttributeId, mockRange, spUIAutomationTextRange.Get(), FlowDirections_Default, FlowDirections_Vertical);
            VerifyGetAttributeValueEnum(L"---UIA_UnderlineColorAttributeId---", UIA_UnderlineColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 190);
            VerifyGetAttributeValueEnum(L"---UIA_UnderlineStyleAttributeId---", UIA_UnderlineStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_None, TextDecorationLineStyle_LongDash);
            VerifyGetAttributeValueIntArray(L"---UIA_AnnotationTypesAttributeId---", UIA_AnnotationTypesAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyGetAttributeValueAPArray(L"---UIA_AnnotationObjectsAttributeId---", UIA_AnnotationObjectsAttributeId, mockRange, spUIAutomationTextRange.Get(), mockAPAsAutomationPeer1);
            VerifyGetAttributeValueString(L"---UIA_StyleNameAttributeId---", UIA_StyleNameAttributeId, mockRange, spUIAutomationTextRange.Get(), L"MyStyle1", L"MyStyle2");
            VerifyGetAttributeValueEnum(L"---UIA_StyleIdAttributeId---", UIA_StyleIdAttributeId, mockRange, spUIAutomationTextRange.Get(), 0, 1234);
            VerifyGetAttributeValueIUnknown(L"---UIA_LinkAttributeId---", UIA_LinkAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyGetAttributeValueBool(L"---UIA_IsActiveAttributeId---", UIA_IsActiveAttributeId, mockRange, spUIAutomationTextRange.Get(), false, true);
            VerifyGetAttributeValueEnum(L"---UIA_SelectionActiveEndAttributeId---", UIA_SelectionActiveEndAttributeId, mockRange, spUIAutomationTextRange.Get(), ActiveEnd_None, ActiveEnd_End);
            VerifyGetAttributeValueEnum(L"---UIA_CaretPositionAttributeId---", UIA_CaretPositionAttributeId, mockRange, spUIAutomationTextRange.Get(), CaretPosition_Unknown, CaretPosition_EndOfLine);
            VerifyGetAttributeValueEnum(L"---UIA_CaretBidiModeAttributeId---", UIA_CaretBidiModeAttributeId, mockRange, spUIAutomationTextRange.Get(), CaretBidiMode_LTR, CaretBidiMode_RTL);

            mockRange->initialize();
        });
    }



    inline void VerifyFindAttributeEnum(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, int valueInitial)
    {
        WEX::Logging::Log::Comment(pMsg);

        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
        AutoVariant varData;
        varData.Storage()->vt = VT_I4;
        varData.Storage()->lVal = valueInitial;
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateInt32(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeBool(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, bool valueInitial)
    {
        WEX::Logging::Log::Comment(pMsg);

        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
        AutoVariant varData;
        varData.Storage()->vt = VT_BOOL;
        varData.Storage()->boolVal = valueInitial ? VARIANT_TRUE : VARIANT_FALSE;
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateBoolean(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeDouble(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, double valueInitial)
    {
        WEX::Logging::Log::Comment(pMsg);

        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
        AutoVariant varData;
        varData.Storage()->vt = VT_R8;
        varData.Storage()->dblVal = valueInitial;
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        Platform::Object^ prop;
        prop = ::Windows::Foundation::PropertyValue::CreateDouble(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeString(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, const wchar_t* valueInitial)
    {
        WEX::Logging::Log::Comment(pMsg);
        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
        AutoVariant varData;
        varData.Storage()->vt = VT_BSTR;
        varData.Storage()->bstrVal = ::SysAllocString(valueInitial);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        Platform::Object^ prop;
        prop = ref new Platform::String(valueInitial);
        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeDoubleArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);
        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;

        SAFEARRAY* pSafeArray = nullptr;
        pSafeArray = SafeArrayCreateVector(VT_R8, 0, 5);
        for (LONG index = 0; index < 5; index++)
        {
            double value = (double) (5 - index);
            VERIFY_SUCCEEDED(SafeArrayPutElement(pSafeArray, &index, &value));
        }

        AutoVariant varData;
        varData.Storage()->vt = VT_ARRAY | VT_R8;
        varData.Storage()->parray = pSafeArray;
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        wfc::IVector<double>^ vec = ref new Platform::Collections::Vector<double>(0);
        vec->Append(5); vec->Append(4); vec->Append(3); vec->Append(2); vec->Append(1);
        wfc::IVectorView<double>^ vecview = vec->GetView();
        Platform::Object^ prop = vecview;

        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeIntArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);
        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;

        SAFEARRAY* pSafeArray = nullptr;
        pSafeArray = SafeArrayCreateVector(VT_I4, 0, 5);
        for (LONG index = 0; index < 5; index++)
        {
            int value = (int)(5 - index);
            VERIFY_SUCCEEDED(SafeArrayPutElement(pSafeArray, &index, &value));
        }

        AutoVariant varData;
        varData.Storage()->vt = VT_ARRAY | VT_I4;
        varData.Storage()->parray = pSafeArray;
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NULL(spUIAutomationTextRange.Get());

        wfc::IVector<int>^ vec = ref new Platform::Collections::Vector<int>(0);
        vec->Append(5); vec->Append(4); vec->Append(3); vec->Append(2); vec->Append(1);
        wfc::IVectorView<int>^ vecview = vec->GetView();
        Platform::Object^ prop = vecview;

        testTextRange->setAttribute(attributeid, prop);
        VERIFY_SUCCEEDED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
        VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
    }

    inline void VerifyFindAttributeAPArray(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern, xaml_automation_peers::AutomationPeer^ ap, _In_ IUIAutomationElement* pAutomationElement)
    {
        WEX::Logging::Log::Comment(pMsg);
        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;

        SAFEARRAY* pSafeArray = nullptr;
        pSafeArray = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
        LONG index = 0;
        VERIFY_SUCCEEDED(SafeArrayPutElement(pSafeArray, &index, pAutomationElement));

        AutoVariant varData;
        varData.Storage()->vt = VT_ARRAY | VT_UNKNOWN;
        varData.Storage()->parray = pSafeArray;
        VERIFY_FAILED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
    }

    inline void VerifyFindAttributeIUnknown(const wchar_t* pMsg, TEXTATTRIBUTEID attributeid, MockUpProviderControlRange^ testTextRange, _In_ IUIAutomationTextRange *pUITextPattern)
    {
        WEX::Logging::Log::Comment(pMsg);

        wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
        AutoVariant varData;
        varData.Storage()->vt = VT_UNKNOWN;
        varData.Storage()->punkVal = pUITextPattern; pUITextPattern->AddRef();  // Variant clear will release one reference
        VERIFY_FAILED(pUITextPattern->FindAttribute(attributeid, varData.Get(), TRUE, &spUIAutomationTextRange));
    }

    void TextRangeIntegrationTests::VerifyFindAttribute()
    {
        TestCleanupWrapper cleanup;
        MockUpProviderControl^ testTextRange = nullptr;
        Platform::Object^ obj = nullptr;
        Platform::Object^ result = nullptr;

        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TestMock";
        uiaInfo.m_AutomationID = L"TestMock";
        uiaInfo.m_cType = UIA_ButtonControlTypeId;
        RunOnUIThread([&]()
        {
            testTextRange = ref new MockUpProviderControl();
            xaml_automation::AutomationProperties::SetName(testTextRange, ref new Platform::String(uiaInfo.m_Name));
            testTextRange->Name = "TestMock";
            testTextRange->FontSize = 30.0;
            WEX::Logging::Log::Comment(L"Adding control to UI");
        });

        TreeHelper::AddElementIntoLivetree<xaml_controls::Button>(testTextRange, true /*wrapInGrid*/);
        TestServices::WindowHelper->WaitForIdle();

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            MockUpProviderControlRange^ mockRange;
            MockUpProviderControlAutomationPeer^ mockAP;
            xaml_automation_peers::AutomationPeer^ mockAPAsAutomationPeer;
            wrl::ComPtr<IUIAutomationElement> spUIAutomationElement;
            wrl::ComPtr<IUIAutomationTextRange> spUIAutomationTextRange;
            wrl::ComPtr<IUIAutomationTextPattern> spUITextPattern;

            WEX::Logging::Log::Comment(L"Executing test on UI thread");

            RunOnUIThread([&]()
            {
                mockAPAsAutomationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(testTextRange);
            });
            VERIFY_IS_NOT_NULL(mockAPAsAutomationPeer);
            mockAP = static_cast<MockUpProviderControlAutomationPeer^>(mockAPAsAutomationPeer);
            VERIFY_IS_NOT_NULL(mockAP);
            mockRange = mockAP->_getRange();
            VERIFY_IS_NOT_NULL(mockRange);
            mockRange->initialize();

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            VERIFY_IS_NOT_NULL(spAutomationClientManager.get());
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_IS_NOT_NULL(spUIAutomationElement.Get());

            WEX::Common::Throw::IfNull(spUIAutomationElement.Get());
            LogThrow_IfFailedWithMessage(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spUITextPattern), L"TextRangeIntegrationTests::VerifyTextAttrributeIds: Failed in retreiving Text Pattern.");
            WEX::Common::Throw::IfNull(spUITextPattern.Get(), L"TextRangeIntegrationTests::VerifyTextAttrributeIds: This element doesn't support Text Pattern which is required.");

            VERIFY_SUCCEEDED(spUITextPattern->get_DocumentRange(&spUIAutomationTextRange));
            VERIFY_IS_NOT_NULL(spUIAutomationTextRange.Get());
            VerifyFindAttributeEnum(L"---UIA_AnimationStyleAttributeId---", UIA_AnimationStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), AnimationStyle_None);
            VerifyFindAttributeEnum(L"---UIA_BackgroundColorAttributeId---", UIA_BackgroundColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 10);
            VerifyFindAttributeEnum(L"---UIA_BulletStyleAttributeId---", UIA_BulletStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), BulletStyle_None);
            VerifyFindAttributeEnum(L"---UIA_CapStyleAttributeId---", UIA_CapStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), CapStyle_None);
            VerifyFindAttributeEnum(L"---UIA_CultureAttributeId---", UIA_CultureAttributeId, mockRange, spUIAutomationTextRange.Get(), 20);
            VerifyFindAttributeString(L"---UIA_FontNameAttributeId---", UIA_FontNameAttributeId, mockRange, spUIAutomationTextRange.Get(), L"Arial");
            VerifyFindAttributeDouble(L"---UIA_FontSizeAttributeId---", UIA_FontSizeAttributeId, mockRange, spUIAutomationTextRange.Get(), 16);
            VerifyFindAttributeEnum(L"---UIA_FontWeightAttributeId---", UIA_FontWeightAttributeId, mockRange, spUIAutomationTextRange.Get(), 0);
            VerifyFindAttributeEnum(L"---UIA_ForegroundColorAttributeId---", UIA_ForegroundColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 40);
            VerifyFindAttributeEnum(L"---UIA_HorizontalTextAlignmentAttributeId---", UIA_HorizontalTextAlignmentAttributeId, mockRange, spUIAutomationTextRange.Get(), HorizontalTextAlignment_Left);
            VerifyFindAttributeDouble(L"---UIA_IndentationFirstLineAttributeId---", UIA_IndentationFirstLineAttributeId, mockRange, spUIAutomationTextRange.Get(), 50);
            VerifyFindAttributeDouble(L"---UIA_IndentationLeadingAttributeId---", UIA_IndentationLeadingAttributeId, mockRange, spUIAutomationTextRange.Get(), 60);
            VerifyFindAttributeDouble(L"---UIA_IndentationTrailingAttributeId---", UIA_IndentationTrailingAttributeId, mockRange, spUIAutomationTextRange.Get(), 70);
            VerifyFindAttributeBool(L"---UIA_IsHiddenAttributeId---", UIA_IsHiddenAttributeId, mockRange, spUIAutomationTextRange.Get(), false);
            VerifyFindAttributeBool(L"---UIA_IsItalicAttributeId---", UIA_IsItalicAttributeId, mockRange, spUIAutomationTextRange.Get(), false);
            VerifyFindAttributeBool(L"---UIA_IsReadOnlyAttributeId---", UIA_IsReadOnlyAttributeId, mockRange, spUIAutomationTextRange.Get(), false);
            VerifyFindAttributeBool(L"---UIA_IsSubscriptAttributeId---", UIA_IsSubscriptAttributeId, mockRange, spUIAutomationTextRange.Get(), false);
            VerifyFindAttributeBool(L"---UIA_IsSuperscriptAttributeId---", UIA_IsSuperscriptAttributeId, mockRange, spUIAutomationTextRange.Get(), false);
            VerifyFindAttributeDouble(L"---UIA_MarginBottomAttributeId---", UIA_MarginBottomAttributeId, mockRange, spUIAutomationTextRange.Get(), 80);
            VerifyFindAttributeDouble(L"---UIA_MarginLeadingAttributeId---", UIA_MarginLeadingAttributeId, mockRange, spUIAutomationTextRange.Get(), 90);
            VerifyFindAttributeDouble(L"---UIA_MarginTopAttributeId---", UIA_MarginTopAttributeId, mockRange, spUIAutomationTextRange.Get(), 600);
            VerifyFindAttributeDouble(L"---UIA_MarginTrailingAttributeId---", UIA_MarginTrailingAttributeId, mockRange, spUIAutomationTextRange.Get(), 700);
            VerifyFindAttributeEnum(L"---UIA_OutlineStylesAttributeId---", UIA_OutlineStylesAttributeId, mockRange, spUIAutomationTextRange.Get(), OutlineStyles_Embossed);
            VerifyFindAttributeEnum(L"---UIA_OverlineColorAttributeId---", UIA_OverlineColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 120);
            VerifyFindAttributeEnum(L"---UIA_OverlineStyleAttributeId---", UIA_OverlineStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_DashDot);
            VerifyFindAttributeEnum(L"---UIA_StrikethroughColorAttributeId---", UIA_StrikethroughColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 220);
            VerifyFindAttributeEnum(L"---UIA_StrikethroughStyleAttributeId---", UIA_StrikethroughStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_Double);
            VerifyFindAttributeDoubleArray(L"---UIA_TabsAttributeId---", UIA_TabsAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyFindAttributeEnum(L"---UIA_TextFlowDirectionsAttributeId---", UIA_TextFlowDirectionsAttributeId, mockRange, spUIAutomationTextRange.Get(), FlowDirections_Vertical);
            VerifyFindAttributeEnum(L"---UIA_UnderlineColorAttributeId---", UIA_UnderlineColorAttributeId, mockRange, spUIAutomationTextRange.Get(), 190);
            VerifyFindAttributeEnum(L"---UIA_UnderlineStyleAttributeId---", UIA_UnderlineStyleAttributeId, mockRange, spUIAutomationTextRange.Get(), TextDecorationLineStyle_LongDash);
            VerifyFindAttributeIntArray(L"---UIA_AnnotationTypesAttributeId---", UIA_AnnotationTypesAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyFindAttributeAPArray(L"---UIA_AnnotationObjectsAttributeId---", UIA_AnnotationObjectsAttributeId, mockRange, spUIAutomationTextRange.Get(), mockAPAsAutomationPeer, spUIAutomationElement.Get());
            VerifyFindAttributeString(L"---UIA_StyleNameAttributeId---", UIA_StyleNameAttributeId, mockRange, spUIAutomationTextRange.Get(), L"MyStyle2");
            VerifyFindAttributeEnum(L"---UIA_StyleIdAttributeId---", UIA_StyleIdAttributeId, mockRange, spUIAutomationTextRange.Get(), 1234);
            VerifyFindAttributeIUnknown(L"---UIA_LinkAttributeId---", UIA_LinkAttributeId, mockRange, spUIAutomationTextRange.Get());
            VerifyFindAttributeBool(L"---UIA_IsActiveAttributeId---", UIA_IsActiveAttributeId, mockRange, spUIAutomationTextRange.Get(), true);
            VerifyFindAttributeEnum(L"---UIA_SelectionActiveEndAttributeId---", UIA_SelectionActiveEndAttributeId, mockRange, spUIAutomationTextRange.Get(), ActiveEnd_End);
            VerifyFindAttributeEnum(L"---UIA_CaretPositionAttributeId---", UIA_CaretPositionAttributeId, mockRange, spUIAutomationTextRange.Get(), CaretPosition_EndOfLine);
            VerifyFindAttributeEnum(L"---UIA_CaretBidiModeAttributeId---", UIA_CaretBidiModeAttributeId, mockRange, spUIAutomationTextRange.Get(), CaretBidiMode_RTL);

            mockRange->initialize();
        });
    }

    void TextRangeIntegrationTests::TestCompare()
    {
        TestCleanupWrapper cleanup;

        MockUpProviderControlAutomationPeer^ mockAP;
        Automation::AutomationClient::UIAElementInfo uiaInfo;
        uiaInfo.m_Name = L"TextRangeIntegrationTests";

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

            auto spAutomationClientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            spAutomationClientManager->GetCurrentUIAutomationElement(&spUIAutomationElement);
            VERIFY_SUCCEEDED(spUIAutomationElement->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &spTextPattern), L"Failed in retreiving TextPattern.");

            mockAP->MakeRangeValid(true);
            auto range = mockAP->_getRange();

            // A mock implementation for ITextRangeProvider::Compare
            bool valueToReturnForCompare = true;
            xaml_automation::Provider::ITextRangeProvider^ otherRangeProvidedFromParam;
            range->CompareImpl = ref new ITextRangeProviderCompare([&](auto otherRange)
            {
                otherRangeProvidedFromParam = otherRange;
                return valueToReturnForCompare;
            });

            // Get an IUIAutomationTextRange
            wrl::ComPtr<IUIAutomationTextRange> iuiautomationTextRange = nullptr;
            VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&iuiautomationTextRange));

            // Get another IUIAutomationTextRange to compare to
            auto range2 = ref new MockUpProviderControlRange();
            mockAP->_setRange(range2);
            wrl::ComPtr<IUIAutomationTextRange> iuiautomationTextRange2 = nullptr;
            VERIFY_SUCCEEDED(spTextPattern->get_DocumentRange(&iuiautomationTextRange2));

            BOOL areSame;
            valueToReturnForCompare = true;
            VERIFY_SUCCEEDED(iuiautomationTextRange->Compare(iuiautomationTextRange2.Get(), &areSame));
            VERIFY_ARE_EQUAL(!!areSame, valueToReturnForCompare);
            VERIFY_IS_TRUE(otherRangeProvidedFromParam == range2);

            valueToReturnForCompare = false;
            VERIFY_SUCCEEDED(iuiautomationTextRange->Compare(iuiautomationTextRange2.Get(), &areSame));
            VERIFY_ARE_EQUAL(!!areSame, valueToReturnForCompare);
            VERIFY_IS_TRUE(otherRangeProvidedFromParam == range2);

            //try null
            VERIFY_FAILED(iuiautomationTextRange->Compare(nullptr, &areSame));

            //try self
            VERIFY_SUCCEEDED(iuiautomationTextRange->Compare(iuiautomationTextRange.Get(), &areSame));
            VERIFY_ARE_EQUAL(!!areSame, valueToReturnForCompare);
            VERIFY_IS_TRUE(otherRangeProvidedFromParam == range);
        });
    }

} } } } } }
