// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppAnalysisCommon.h"
#include "XamlLogging.h"
#include "MockTDH.h"
#include "EventProcessorTests.h"
#include "resource.h"
#include <ppltasks.h>
#include "wil\resource.h"
#include "RuleTester.h"
#include <TestEvent.h>
#include "EventProcessor.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace AppAnalysis::Test;
using namespace WEX::Common;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {
    namespace ScreenCapture {
        // This isn't supported because taking a screenshot in an isolated test doesn't make much sense
        // because currently these tests don't render anything to the screen. If this changes, maybe we
        // find a way to share the actual implementation that's in WindowingHelper.cpp (or even better, see
        // if TAEF has a single API that supports screenshots across platforms and get rid of ours).
        void TakeScreenshot(const wchar_t*)
        {
            WEX::Logging::Log::Warning(L"Trying to take screenshot in an isolated test is not supported");
        }
    }
} } } } }


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {  namespace Tools { namespace AppAnalysis {

void EventProcessorTests::ValidateTraceLoggingVirtualizationDispatch()
{
    namespace engine = Microsoft::Diagnostics::AppAnalysis;
    wrl::ComPtr<engine::EventProcessor> processor;
    VERIFY_SUCCEEDED(wrl::MakeAndInitialize<engine::EventProcessor>(&processor));
    wrl::ComPtr<appanalysis::IEventProcessorStatics> registrations;
    VERIFY_SUCCEEDED(processor.As(&registrations));
    wrl::ComPtr<engine::IEventProcessorPrivate> events;
    VERIFY_SUCCEEDED(processor.As(&events));

    // Registration only needs a provider identity; avoid loading an external manifest.
    wrl::ComPtr<engine::EtwProvider> provider;
    VERIFY_SUCCEEDED(wrl::MakeAndInitialize<engine::EtwProvider>(&provider, WINDOWS_UI_XAML_ETW_PROVIDER, UINT32{0}));
    wrl::ComPtr<engine::EtwEvent> observation;
    VERIFY_SUCCEEDED(engine::EtwEvent::CreateInstance(
        { VirtualizationIsEnabledByLayoutInfo_value, 1 }, provider.Get(), &observation));

    unsigned int count = 0;
    auto callback = wrl::Callback<appanalysis::IEtwEventRecordCallback>(
        [&](appanalysis::IEtwEventRecord*) { ++count; return S_OK; });
    VERIFY_SUCCEEDED(registrations->RegisterEvent(observation.Get(), callback.Get()));

    // TraceLogging metadata is a two-byte size, a tag byte, and a terminated event name.
    const char name[] = "VirtualizationIsEnabledByLayout";
    std::vector<BYTE> schema(3 + sizeof(name));
    schema[0] = static_cast<BYTE>(schema.size());
    memcpy(schema.data() + 3, name, sizeof(name));
    const char providerName[] = "Microsoft-Windows-XAML";
    std::vector<BYTE> traits(2 + sizeof(providerName));
    traits[0] = static_cast<BYTE>(traits.size());
    memcpy(traits.data() + 2, providerName, sizeof(providerName));
    EVENT_HEADER_EXTENDED_DATA_ITEM extensions[2]{};
    extensions[0].ExtType = EVENT_HEADER_EXT_TYPE_EVENT_SCHEMA_TL;
    extensions[0].DataSize = static_cast<USHORT>(schema.size());
    extensions[0].DataPtr = reinterpret_cast<ULONGLONG>(schema.data());
    extensions[1].ExtType = EVENT_HEADER_EXT_TYPE_PROV_TRAITS;
    extensions[1].DataSize = static_cast<USHORT>(traits.size());
    extensions[1].DataPtr = reinterpret_cast<ULONGLONG>(traits.data());

    EVENT_RECORD record{};
    record.EventHeader.Size = sizeof(EVENT_HEADER);
    record.EventHeader.ProviderId = WINDOWS_UI_XAML_ETW_PROVIDER;
    record.EventHeader.Flags = EVENT_HEADER_FLAG_64_BIT_HEADER | EVENT_HEADER_FLAG_EXTENDED_INFO;
    record.EventHeader.EventDescriptor.Channel = 11;
    record.ExtendedDataCount = _countof(extensions);
    record.ExtendedData = extensions;
    VERIFY_SUCCEEDED(events->ProcessEvent(&record));
    VERIFY_ARE_EQUAL(1u, count);

    schema[3] = 'X';
    VERIFY_SUCCEEDED(events->ProcessEvent(&record));
    VERIFY_ARE_EQUAL(1u, count);

    record.ExtendedDataCount = 0;
    VERIFY_SUCCEEDED(events->ProcessEvent(&record));
    VERIFY_ARE_EQUAL(1u, count);

    record.EventHeader.EventDescriptor.Id = VirtualizationIsEnabledByLayoutInfo_value;
    record.EventHeader.EventDescriptor.Version = 1;
    VERIFY_SUCCEEDED(events->ProcessEvent(&record));
    VERIFY_ARE_EQUAL(2u, count);
}

////////////////////////////////////////////////////////////////////////////////
// ValidateBasicProperties: Tests that properties on ETWEvent work for the golden path.
//
void EventProcessorTests::ValidateBasicProperties()
{
    std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

    RuleNotificationCallback notificationCallback = RuleNotificationCallback([&notificationEvent](appanalysis::IRule*, appanalysis::IRuleTriggeredEventArgs*){
        LOG_OUTPUT(L"Woooh we got the notification!");
        notificationEvent->Set();
        return S_OK;
    });

    RuleTester ruleTester;
    ruleTester.EnableTestRule(L"Rule001");

    ruleTester.SetCallback(notificationCallback);
    ruleTester.ProcessMockEvent(CreateMockEvent(InitializeCoreBegin_value, WINDOWS_UI_XAML_ETW_PROVIDER, 1, 0));
    ruleTester.ProcessMockEvent(CreateMockEvent(ApplicationStartupInfo_value, WINDOWS_UI_XAML_ETW_PROVIDER, LONGLONG_MAX, 0));

    notificationEvent->WaitForDefault();
    VERIFY_IS_TRUE(notificationEvent->HasFired());
}

void EventProcessorTests::ValidateRuleDoesntGetCallbackTwice()
{
    // Setup the callback, this is where we verify all the data that we got
    std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

    unsigned int count = 0;
    RuleNotificationCallback notificationCallback = RuleNotificationCallback(
        [&](appanalysis::IRule*, appanalysis::IRuleTriggeredEventArgs*) {

        count++;
        if (count > 1)
        {
            notificationEvent->Set();
        }
        return S_OK;
    });

    // Create RuleTester and add rule to App Analysis engine. We will use the image decoding rule for this
    RuleTester ruleTester;
    ruleTester.EnableTestRule(L"Rule003");

    ruleTester.SetCallback(notificationCallback);
    ruleTester.ProcessMockEvent(CreateMockEvent(ParseXamlBegin_value, WINDOWS_UI_XAML_ETW_PROVIDER, 5000, 0));

    // wait 3 seconds but don't throw
    notificationEvent->WaitForNoThrow(std::chrono::milliseconds(3000));
    VERIFY_IS_FALSE(notificationEvent->HasFired());
    VERIFY_ARE_EQUAL(count, 1u);
}


} } } } } }