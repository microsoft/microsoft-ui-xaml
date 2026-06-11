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