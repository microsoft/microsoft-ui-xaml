// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppAnalysisCommon.h"
#include "XamlLogging.h"
#include "MockTDH.h"
#include "EtwEventRecordTests.h"
#include "EtwEventRecord.h"
namespace Windows { namespace UI { namespace Xaml { namespace Tests {  namespace Tools { namespace AppAnalysis {


////////////////////////////////////////////////////////////////////////////////
// ValidateBasicProperties: Tests that properties on ETWEvent work for the golden path.
//
void ETWEventTests::ValidateBasicProperties()
{
    wrl::ComPtr<appanalysis_impl::EtwEventRecord> etwEvent;
    // Initialize the Mock TDH object with canned values.
    EVENT_RECORD eventRecord = CreateEventRecord(12345678, WINDOWS_UI_XAML_ETW_PROVIDER, 300, 3);

    VERIFY_SUCCEEDED(wrl::MakeAndInitialize<appanalysis_impl::EtwEventRecord>(&etwEvent));

    etwEvent->SetEventRecord(&eventRecord);

    // TimeStamp
    {
        LONGLONG llTimeStamp = 0;
        VERIFY_SUCCEEDED(etwEvent->get_Timestamp(&llTimeStamp));
        VERIFY_ARE_EQUAL(llTimeStamp, 12345678);
    }

    // EtwEventInfo
    {
        appanalysis_impl::EventInfo info = { 0 };
        GUID providerId = GUID_NULL;
        VERIFY_SUCCEEDED(etwEvent->get_EventId(&info.EventId));
        VERIFY_SUCCEEDED(etwEvent->get_EventVersion(&info.EventVersion));
        VERIFY_SUCCEEDED(etwEvent->get_ProviderId(&providerId));

        VERIFY_ARE_EQUAL(info.EventId, 300);
        VERIFY_ARE_EQUAL(info.EventVersion, 3);
        VERIFY_IS_TRUE(!!IsEqualGUID(providerId, WINDOWS_UI_XAML_ETW_PROVIDER));
    }

    // GetUnicodeStringProperty
    {
        const TCHAR szUnicodePropertyValue[] = L"Oak is strong and also gives shade";
        LPWSTR propertyName = L"StringProperty";

        MockTDHSetProperty(propertyName, (BYTE*)szUnicodePropertyValue, sizeof(szUnicodePropertyValue));

        wil::unique_hstring propertyValue;

        VERIFY_SUCCEEDED(etwEvent->GetUnicodeStringProperty(StringRef(propertyName), &propertyValue));
        VERIFY_IS_TRUE(AppAnalysisHelpers::CompareStrings(StringRef(szUnicodePropertyValue),propertyValue.get()) == 0);
    }

    // GetBooleanProperty
    {
        BOOL valueIn = true;
        boolean valueOut = false;
        LPWSTR testProperty = L"BoolProperty";

        MockTDHSetProperty(testProperty, (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetBooleanProperty(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }

    // GetInt32Property
    {
        INT valueIn = 123;
        int valueOut = 0;
        LPWSTR testProperty = L"Int32Property";

        MockTDHSetProperty(testProperty, (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetInt32Property(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }

    // GetUInt32Property
    {
        UINT valueIn = 1234;
        UINT valueOut = 0;
        LPWSTR testProperty = L"UINTProperty";

        MockTDHSetProperty(L"UINTProperty", (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetUInt32Property(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }

    // GetInt64Property
    {
        LONGLONG valueIn = 12345;
        LONGLONG valueOut = 0;
        LPWSTR testProperty = L"Int64Property";

        MockTDHSetProperty(L"Int64Property", (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetInt64Property(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }

    // GetUInt64Property
    {
        ULONGLONG valueIn = 123456;
        ULONGLONG valueOut = 0;
        LPWSTR testProperty = L"UInt64Property";

        MockTDHSetProperty(L"UInt64Property", (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetUInt64Property(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }

    // GetFloatProperty
    {
        float valueIn = 123.456f;
        float valueOut = 0.0f;
        LPWSTR testProperty = L"FloatProperty";

        MockTDHSetProperty(L"FloatProperty", (BYTE*)&valueIn, sizeof(valueIn));
        VERIFY_SUCCEEDED(etwEvent->GetFloatProperty(StringRef(testProperty), &valueOut));
        VERIFY_ARE_EQUAL(valueIn, valueOut);
    }
}

} } } } } }