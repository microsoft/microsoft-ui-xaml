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
#include <vector>
#include <string>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Tests::Automation;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Tests::Automation::Patterns;

#pragma warning(push)
#pragma warning(disable: 4100) // Unreferenced parameters are legal for formal interface implementations.

namespace TextRangeMocks {

    namespace xap = ::Microsoft::UI::Xaml::Automation::Provider;
    using TextEndpoint = ::Microsoft::UI::Xaml::Automation::Text::TextPatternRangeEndpoint;
    using TextUnit = ::Microsoft::UI::Xaml::Automation::Text::TextUnit;

    struct FindTextCall
    {
        std::wstring text;
        bool backward;
        bool ignoreCase;
    };

    // Builds the color directly; Windows::UI::ColorHelper would load Windows.UI.Xaml.dll into the test process.
    inline ::Windows::UI::Color MakeColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
    {
        ::Windows::UI::Color color = {};
        color.A = a;
        color.R = r;
        color.G = g;
        color.B = b;
        return color;
    }

    // Text range provider that records every call it receives and returns distinct values.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class RecordingTextRange sealed : public xap::ITextRangeProvider2
    {
    public:
        RecordingTextRange(Platform::String^ label)
            : m_label(label)
            , m_returnNullClone(false)
            , m_getEnclosingElementCount(0)
            , m_addToSelectionCount(0)
            , m_removeFromSelectionCount(0)
            , m_showContextMenuCount(0)
        {
        }

        property Platform::String^ Label { Platform::String^ get() { return m_label; } }

        virtual xap::ITextRangeProvider^ Clone()
        {
            return m_returnNullClone ? nullptr : ref new RecordingTextRange(L"clone");
        }

        virtual bool Compare(xap::ITextRangeProvider^ textRangeProvider) { return textRangeProvider == this; }

        virtual int CompareEndpoints(TextEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, TextEndpoint targetEndpoint)
        {
            auto target = dynamic_cast<RecordingTextRange^>(textRangeProvider);
            m_compareEndpointsCalls.push_back(std::make_pair(endpoint, targetEndpoint));
            m_compareEndpointsTargets.push_back(target ? std::wstring(target->Label->Data()) : std::wstring(L"<unexpected>"));
            return -7;
        }

        virtual void ExpandToEnclosingUnit(TextUnit unit) {}

        virtual xap::ITextRangeProvider^ FindAttribute(int attributeId, Platform::Object^ value, bool backward) { return nullptr; }

        virtual xap::ITextRangeProvider^ FindText(Platform::String^ text, bool backward, bool ignoreCase)
        {
            m_findTextCalls.push_back(FindTextCall{ text ? std::wstring(text->Data()) : std::wstring(), backward, ignoreCase });
            if (Platform::String::CompareOrdinal(text, L"abc") == 0)
            {
                return ref new RecordingTextRange(L"found");
            }
            return nullptr;
        }

        virtual Platform::Object^ GetAttributeValue(int attributeId)
        {
            switch (attributeId)
            {
            case UIA_ForegroundColorAttributeId:
                return ref new xaml_media::SolidColorBrush(MakeColor(0xFF, 0x12, 0x34, 0x56));
            case UIA_UnderlineColorAttributeId:
                // A brush that is not a SolidColorBrush has no single color.
                return ref new xaml_media::LinearGradientBrush();
            case UIA_TabsAttributeId:
                return (ref new Platform::Collections::Vector<double>())->GetView();
            case UIA_AnnotationTypesAttributeId:
                return (ref new Platform::Collections::Vector<int>())->GetView();
            default:
                return nullptr;
            }
        }

        virtual void GetBoundingRectangles(Platform::Array<double>^* returnValue)
        {
            auto rectangles = ref new Platform::Array<double>(8);
            for (unsigned int i = 0; i < rectangles->Length; ++i)
            {
                rectangles[i] = 10.0 * (i + 1);
            }
            *returnValue = rectangles;
        }

        virtual xap::IRawElementProviderSimple^ GetEnclosingElement()
        {
            ++m_getEnclosingElementCount;
            return m_enclosingElement;
        }

        virtual Platform::String^ GetText(int maxLength) { return m_label; }
        virtual int Move(TextUnit unit, int count) { return 0; }
        virtual int MoveEndpointByUnit(TextEndpoint endpoint, TextUnit unit, int count) { return 0; }
        virtual void MoveEndpointByRange(TextEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, TextEndpoint targetEndpoint) {}
        virtual void Select() {}
        virtual void AddToSelection() { ++m_addToSelectionCount; }
        virtual void RemoveFromSelection() { ++m_removeFromSelectionCount; }
        virtual void ScrollIntoView(bool alignToTop) { m_scrollIntoViewCalls.push_back(alignToTop); }
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetChildren() { return ref new Platform::Array<xap::IRawElementProviderSimple^>(0); }

        // ITextRangeProvider2
        virtual void ShowContextMenu() { ++m_showContextMenuCount; }

    internal:
        bool m_returnNullClone;
        xap::IRawElementProviderSimple^ m_enclosingElement;
        std::vector<std::pair<TextEndpoint, TextEndpoint>> m_compareEndpointsCalls;
        std::vector<std::wstring> m_compareEndpointsTargets;
        std::vector<FindTextCall> m_findTextCalls;
        int m_getEnclosingElementCount;
        int m_addToSelectionCount;
        int m_removeFromSelectionCount;
        std::vector<bool> m_scrollIntoViewCalls;
        int m_showContextMenuCount;

    private:
        Platform::String^ m_label;
    };

    // Text range provider that does not implement ITextRangeProvider2.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class PlainTextRange sealed : public xap::ITextRangeProvider
    {
    public:
        virtual xap::ITextRangeProvider^ Clone() { return ref new PlainTextRange(); }
        virtual bool Compare(xap::ITextRangeProvider^ textRangeProvider) { return textRangeProvider == this; }
        virtual int CompareEndpoints(TextEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, TextEndpoint targetEndpoint) { return 0; }
        virtual void ExpandToEnclosingUnit(TextUnit unit) {}
        virtual xap::ITextRangeProvider^ FindAttribute(int attributeId, Platform::Object^ value, bool backward) { return nullptr; }
        virtual xap::ITextRangeProvider^ FindText(Platform::String^ text, bool backward, bool ignoreCase) { return nullptr; }
        virtual Platform::Object^ GetAttributeValue(int attributeId) { return nullptr; }
        virtual void GetBoundingRectangles(Platform::Array<double>^* returnValue) { *returnValue = ref new Platform::Array<double>(0); }
        virtual xap::IRawElementProviderSimple^ GetEnclosingElement() { return nullptr; }
        virtual Platform::String^ GetText(int maxLength) { return L"plain"; }
        virtual int Move(TextUnit unit, int count) { return 0; }
        virtual int MoveEndpointByUnit(TextEndpoint endpoint, TextUnit unit, int count) { return 0; }
        virtual void MoveEndpointByRange(TextEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, TextEndpoint targetEndpoint) {}
        virtual void Select() {}
        virtual void AddToSelection() {}
        virtual void RemoveFromSelection() {}
        virtual void ScrollIntoView(bool alignToTop) {}
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetChildren() { return ref new Platform::Array<xap::IRawElementProviderSimple^>(0); }
    };

    // Button peer whose text pattern document range is supplied by the test.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class RecordingRangePeer sealed : public xaml_automation_peers::ButtonAutomationPeer, public xap::ITextProvider
    {
    public:
        RecordingRangePeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner) {}

        virtual Platform::Array<xap::ITextRangeProvider^>^ GetSelection() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual Platform::Array<xap::ITextRangeProvider^>^ GetVisibleRanges() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual xap::ITextRangeProvider^ RangeFromChild(xap::IRawElementProviderSimple^ childElement) { return nullptr; }
        virtual xap::ITextRangeProvider^ RangeFromPoint(::Windows::Foundation::Point screenLocation) { return nullptr; }
        virtual property xap::ITextRangeProvider^ DocumentRange { xap::ITextRangeProvider^ get() { return m_documentRange; } }
        virtual property ::Microsoft::UI::Xaml::Automation::SupportedTextSelection SupportedTextSelection
        {
            ::Microsoft::UI::Xaml::Automation::SupportedTextSelection get() { return ::Microsoft::UI::Xaml::Automation::SupportedTextSelection::Single; }
        }

    internal:
        xap::ITextRangeProvider^ m_documentRange;

        xap::IRawElementProviderSimple^ GetProviderForPeer(xaml_automation_peers::AutomationPeer^ peer)
        {
            return ProviderFromPeer(peer);
        }

    protected:
        Platform::Object^ GetPatternCore(xaml_automation_peers::PatternInterface patternInterface) override
        {
            if (patternInterface == xaml_automation_peers::PatternInterface::Text)
            {
                return this;
            }
            return __super::GetPatternCore(patternInterface);
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class RecordingRangeHostButton sealed : public xaml_controls::Button
    {
    protected:
        xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            return ref new RecordingRangePeer(this);
        }
    };
}

#pragma warning(pop)

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

    namespace
    {
        void VerifyUiaRangeText(IUIAutomationTextRange* range, const wchar_t* expectedText)
        {
            VERIFY_IS_NOT_NULL(range);
            AutoBSTR text;
            VERIFY_SUCCEEDED(range->GetText(-1, text.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(static_cast<bool>(text), L"Expected a non-null BSTR.");
            AutoBSTR::VerifyAreEqual(expectedText, text);
        }

        // XAML marshals an empty Tabs/AnnotationTypes list as VT_EMPTY (CUIAWindow::CValueToVariant), which UIA reports to
        // clients as its reserved NotSupported value. An empty SAFEARRAY of the attribute's documented element type is the
        // other valid representation; any other value (a scalar, another object, the MixedAttribute sentinel, a non-empty or
        // differently typed array) is a marshalling error.
        void VerifyEmptyCollectionAttribute(VARIANT* value, VARTYPE expectedElementType)
        {
            LOG_OUTPUT(L"Attribute variant type: %d.", static_cast<int>(value->vt));
            if ((value->vt & VT_ARRAY) != 0)
            {
                VERIFY_ARE_EQUAL(static_cast<int>(VT_ARRAY | expectedElementType), static_cast<int>(value->vt));
                VERIFY_IS_NOT_NULL(value->parray);
                LONG lower = 0;
                LONG upper = -1;
                VERIFY_SUCCEEDED(SafeArrayGetLBound(value->parray, 1, &lower));
                VERIFY_SUCCEEDED(SafeArrayGetUBound(value->parray, 1, &upper));
                VERIFY_ARE_EQUAL(0, static_cast<int>(upper - lower + 1));
            }
            else
            {
                VERIFY_ARE_EQUAL(static_cast<int>(VT_UNKNOWN), static_cast<int>(value->vt));
                AutomationClient::AutomationClientManager clientManager;
                wrl::ComPtr<IUIAutomation> automation;
                clientManager.GetAutomation(&automation);
                wrl::ComPtr<IUnknown> notSupportedValue;
                VERIFY_SUCCEEDED(automation->get_ReservedNotSupportedValue(&notSupportedValue));
                VERIFY_IS_NOT_NULL(notSupportedValue.Get());
                VERIFY_IS_TRUE(value->punkVal == notSupportedValue.Get());
            }
        }
    }

    void TextRangeIntegrationTests::VerifyCustomTextRangeProviderMarshalling()
    {
        TestCleanupWrapper cleanup;

        TextRangeMocks::RecordingRangeHostButton^ host = nullptr;
        xaml_controls::Button^ enclosingCell = nullptr;
        TextRangeMocks::RecordingRangePeer^ peer = nullptr;
        TextRangeMocks::RecordingTextRange^ documentRange = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                if (documentRange)
                {
                    documentRange->m_enclosingElement = nullptr;
                }
                if (peer)
                {
                    peer->m_documentRange = nullptr;
                }
                documentRange = nullptr;
                peer = nullptr;
                enclosingCell = nullptr;
                host = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();
            host = ref new TextRangeMocks::RecordingRangeHostButton();
            host->Content = ref new Platform::String(L"RangeMock");
            xaml_automation::AutomationProperties::SetName(host, L"RangeMock");
            enclosingCell = ref new xaml_controls::Button();
            enclosingCell->Content = ref new Platform::String(L"EnclosingCell");
            xaml_automation::AutomationProperties::SetName(enclosingCell, L"EnclosingCell");
            root->Children->Append(host);
            root->Children->Append(enclosingCell);
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            peer = static_cast<TextRangeMocks::RecordingRangePeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(host));
            VERIFY_IS_NOT_NULL(peer);
            auto enclosingPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(enclosingCell);
            documentRange = ref new TextRangeMocks::RecordingTextRange(L"document");
            documentRange->m_enclosingElement = peer->GetProviderForPeer(enclosingPeer);
            peer->m_documentRange = documentRange;
        });

        Automation::AutomationClient::UIAElementInfo uiaInfo = {};
        uiaInfo.m_Name = L"RangeMock";

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> element;
            auto clientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            clientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element.Get());

            wrl::ComPtr<IUIAutomationTextPattern> textPattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &textPattern));
            VERIFY_IS_NOT_NULL(textPattern.Get());

            wrl::ComPtr<IUIAutomationTextRange> document;
            VERIFY_SUCCEEDED(textPattern->get_DocumentRange(&document));
            VerifyUiaRangeText(document.Get(), L"document");

            LOG_OUTPUT(L"Clone returns the provider's clone.");
            wrl::ComPtr<IUIAutomationTextRange> clone;
            VERIFY_SUCCEEDED(document->Clone(&clone));
            VerifyUiaRangeText(clone.Get(), L"clone");

            LOG_OUTPUT(L"CompareEndpoints passes both endpoints and the other range, and returns the provider's result.");
            int compareResult = 0;
            VERIFY_SUCCEEDED(document->CompareEndpoints(TextPatternRangeEndpoint_End, clone.Get(), TextPatternRangeEndpoint_Start, &compareResult));
            VERIFY_ARE_EQUAL(-7, compareResult);

            LOG_OUTPUT(L"FindText passes the text and both flags and returns the provider's range.");
            AutoBSTR abc(L"abc");
            wrl::ComPtr<IUIAutomationTextRange> found;
            VERIFY_SUCCEEDED(document->FindText(abc, TRUE, FALSE, &found));
            VerifyUiaRangeText(found.Get(), L"found");
            AutoBSTR zzz(L"zzz");
            const HRESULT notFoundResult = document->FindText(zzz, FALSE, TRUE, found.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"FindText for text the provider does not find returned 0x%08x.", notFoundResult);
            VERIFY_SUCCEEDED(notFoundResult);
            VERIFY_IS_NULL(found.Get());

            LOG_OUTPUT(L"A SolidColorBrush color attribute is reported as a COLORREF (0x00BBGGRR).");
            AutoVariant attribute;
            VERIFY_SUCCEEDED(document->GetAttributeValue(UIA_ForegroundColorAttributeId, attribute.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(static_cast<int>(VT_I4), static_cast<int>(attribute.Storage()->vt));
            VERIFY_ARE_EQUAL(0x563412, static_cast<int>(attribute.Storage()->lVal));

            LOG_OUTPUT(L"A color attribute that is not a SolidColorBrush is reported as 0.");
            VERIFY_SUCCEEDED(document->GetAttributeValue(UIA_UnderlineColorAttributeId, attribute.ReleaseAndGetAddressOf()));
            VERIFY_ARE_EQUAL(static_cast<int>(VT_I4), static_cast<int>(attribute.Storage()->vt));
            VERIFY_ARE_EQUAL(0, static_cast<int>(attribute.Storage()->lVal));

            LOG_OUTPUT(L"Empty collection attributes are reported as empty (an empty typed array or UIA's NotSupported value).");
            VERIFY_SUCCEEDED(document->GetAttributeValue(UIA_TabsAttributeId, attribute.ReleaseAndGetAddressOf()));
            VerifyEmptyCollectionAttribute(attribute.Storage(), VT_R8);
            VERIFY_SUCCEEDED(document->GetAttributeValue(UIA_AnnotationTypesAttributeId, attribute.ReleaseAndGetAddressOf()));
            VerifyEmptyCollectionAttribute(attribute.Storage(), VT_I4);

            LOG_OUTPUT(L"GetBoundingRectangles returns every provider value; sizes and relative offsets are preserved.");
            SAFEARRAY* rectangles = nullptr;
            VERIFY_SUCCEEDED(document->GetBoundingRectangles(&rectangles));
            auto destroyRectangles = wil::scope_exit([&]() { if (rectangles) { SafeArrayDestroy(rectangles); } });
            VERIFY_IS_NOT_NULL(rectangles);
            LONG lower = 0;
            LONG upper = -1;
            VERIFY_SUCCEEDED(SafeArrayGetLBound(rectangles, 1, &lower));
            VERIFY_SUCCEEDED(SafeArrayGetUBound(rectangles, 1, &upper));
            VERIFY_ARE_EQUAL(8, static_cast<int>(upper - lower + 1));
            double values[8] = {};
            for (LONG i = 0; i < 8; ++i)
            {
                LONG index = lower + i;
                VERIFY_SUCCEEDED(SafeArrayGetElement(rectangles, &index, &values[i]));
                LOG_OUTPUT(L"Rectangle value %d = %f", static_cast<int>(i), values[i]);
            }
            VERIFY_ARE_EQUAL(30.0, values[2]);
            VERIFY_ARE_EQUAL(40.0, values[3]);
            VERIFY_ARE_EQUAL(70.0, values[6]);
            VERIFY_ARE_EQUAL(80.0, values[7]);
            VERIFY_ARE_EQUAL(40.0, values[4] - values[0]);
            VERIFY_ARE_EQUAL(40.0, values[5] - values[1]);

            LOG_OUTPUT(L"GetEnclosingElement returns the element for the provider's enclosing element.");
            wrl::ComPtr<IUIAutomationElement> enclosing;
            VERIFY_SUCCEEDED(document->GetEnclosingElement(&enclosing));
            VERIFY_IS_NOT_NULL(enclosing.Get());
            AutoBSTR enclosingName;
            VERIFY_SUCCEEDED(enclosing->get_CurrentName(enclosingName.ReleaseAndGetAddressOf()));
            VERIFY_IS_TRUE(static_cast<bool>(enclosingName));
            AutoBSTR::VerifyAreEqual(L"EnclosingCell", enclosingName);

            LOG_OUTPUT(L"Selection and scrolling calls reach the provider.");
            VERIFY_SUCCEEDED(document->AddToSelection());
            VERIFY_SUCCEEDED(document->RemoveFromSelection());
            VERIFY_SUCCEEDED(document->ScrollIntoView(FALSE));
            VERIFY_SUCCEEDED(document->ScrollIntoView(TRUE));

            LOG_OUTPUT(L"ShowContextMenu reaches an ITextRangeProvider2 provider.");
            wrl::ComPtr<IUIAutomationTextRange2> document2;
            VERIFY_SUCCEEDED(document.As(&document2));
            VERIFY_SUCCEEDED(document2->ShowContextMenu());
        });

        RunOnUIThread([&]()
        {
            documentRange->m_returnNullClone = true;
            peer->m_documentRange = ref new TextRangeMocks::PlainTextRange();
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> element;
            auto clientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            clientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element.Get());

            wrl::ComPtr<IUIAutomationTextPattern> textPattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &textPattern));
            VERIFY_IS_NOT_NULL(textPattern.Get());

            LOG_OUTPUT(L"A provider without ITextRangeProvider2 cannot show a context menu.");
            wrl::ComPtr<IUIAutomationTextRange> plain;
            VERIFY_SUCCEEDED(textPattern->get_DocumentRange(&plain));
            VerifyUiaRangeText(plain.Get(), L"plain");
            wrl::ComPtr<IUIAutomationTextRange2> plain2;
            HRESULT showContextMenuResult = plain.As(&plain2);
            if (SUCCEEDED(showContextMenuResult))
            {
                showContextMenuResult = plain2->ShowContextMenu();
            }
            LOG_OUTPUT(L"ShowContextMenu on a provider without ITextRangeProvider2 returned 0x%08x.", showContextMenuResult);
            VERIFY_IS_TRUE(FAILED(showContextMenuResult));
        });

        RunOnUIThread([&]()
        {
            peer->m_documentRange = documentRange;
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            wrl::ComPtr<IUIAutomationElement> element;
            auto clientManager = AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(uiaInfo);
            clientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element.Get());

            wrl::ComPtr<IUIAutomationTextPattern> textPattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, __uuidof(IUIAutomationTextPattern), &textPattern));
            wrl::ComPtr<IUIAutomationTextRange> document;
            VERIFY_SUCCEEDED(textPattern->get_DocumentRange(&document));

            LOG_OUTPUT(L"Clone for a provider that returns no clone does not produce a range.");
            wrl::ComPtr<IUIAutomationTextRange> clone;
            const HRESULT cloneResult = document->Clone(&clone);
            LOG_OUTPUT(L"Clone returned 0x%08x.", cloneResult);
            VERIFY_SUCCEEDED(cloneResult);
            VERIFY_IS_NULL(clone.Get());
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(documentRange->m_compareEndpointsCalls.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::Text::TextPatternRangeEndpoint::End), static_cast<int>(documentRange->m_compareEndpointsCalls[0].first));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::Text::TextPatternRangeEndpoint::Start), static_cast<int>(documentRange->m_compareEndpointsCalls[0].second));
            VERIFY_IS_TRUE(documentRange->m_compareEndpointsTargets[0] == L"clone");

            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(documentRange->m_findTextCalls.size()));
            VERIFY_IS_TRUE(documentRange->m_findTextCalls[0].text == L"abc");
            VERIFY_IS_TRUE(documentRange->m_findTextCalls[0].backward);
            VERIFY_IS_FALSE(documentRange->m_findTextCalls[0].ignoreCase);
            VERIFY_IS_TRUE(documentRange->m_findTextCalls[1].text == L"zzz");
            VERIFY_IS_FALSE(documentRange->m_findTextCalls[1].backward);
            VERIFY_IS_TRUE(documentRange->m_findTextCalls[1].ignoreCase);

            VERIFY_ARE_EQUAL(1, documentRange->m_getEnclosingElementCount);
            VERIFY_ARE_EQUAL(1, documentRange->m_addToSelectionCount);
            VERIFY_ARE_EQUAL(1, documentRange->m_removeFromSelectionCount);
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(documentRange->m_scrollIntoViewCalls.size()));
            VERIFY_IS_FALSE(documentRange->m_scrollIntoViewCalls[0]);
            VERIFY_IS_TRUE(documentRange->m_scrollIntoViewCalls[1]);
            VERIFY_ARE_EQUAL(1, documentRange->m_showContextMenuCount);
        });
    }

} } } } } }
