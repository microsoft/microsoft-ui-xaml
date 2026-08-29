// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppAnalysisCommon.h"
#include "UIThreadUtilizationRuleTests.h"
#include "RuleTester.h"
#include "StringDefinitions.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace AppAnalysis::Test;
using namespace WEX::Common;
using namespace Microsoft::Diagnostics::AppAnalysis;

typedef std::function < HRESULT(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv)> pfnCreateObject;
namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace AppAnalysis {

    /*
            ValidateTestRulePass: Validates a scenario where the rule failed and an event is fired.
            Rather than replay scenarios where the outcome relies on the framework and regressions (or improvements)
            could cause false passes/fails, an etl file is deployed with the test machine and ran against the tool.
            This will help catch regressions in the AppAnalysis or the rule itself if tests start failing.
    **/
    void UIThreadUtilizationRuleTests::ValidateUIThreadUtilizationMeasurement()
    {
        // Setup the callback
        bool fired = false;

        RuleNotificationCallback notificationCallback = RuleNotificationCallback([&fired](appanalysis::IRule* /*sender*/, appanalysis::IRuleTriggeredEventArgs* info) {
            fired = true;
            appanalysis::Measurement measurement = { 0 };
            LogThrow_IfFailed(info->get_MeasurementUnit(&measurement.Unit));
            LogThrow_IfFailed(info->get_MeasurementValue(&measurement.Value));

            VERIFY_ARE_EQUAL(measurement.Unit, appanalysis::MeasurementUnit_Percentage);
            VERIFY_IS_TRUE(measurement.Value > 78.11 && measurement.Value < 78.12);
            return S_OK;
        });

        // Create RuleTester and add rule to App Analysis engine
        RuleTester ruleTester;
        ruleTester.EnableRule(UI_THREAD_UTILIZATION_ID);

        // Run App Analysis on etl
        ruleTester.SetCallback(notificationCallback);
        ruleTester.ProcessEtl(GetPathToEtl(L"resources\\AppAnalysis\\UIThreadUtilizationRuleFired.etl").c_str());

        VERIFY_IS_TRUE(fired);
    }

    void UIThreadUtilizationRuleTests::ValidateUIThreadUtilizationNoFire()
    {
        // Setup the callback
        bool fired = false;

        RuleNotificationCallback notificationCallback = RuleNotificationCallback([&fired](appanalysis::IRule*, appanalysis::IRuleTriggeredEventArgs*) {
            fired = true;
            return S_OK;
        });

        // Create RuleTester and add rule to App Analysis engine
        RuleTester ruleTester;
        ruleTester.EnableRule(UI_THREAD_UTILIZATION_ID);

        // Run App Analysis on etl
        ruleTester.SetCallback(notificationCallback);
        ruleTester.ProcessEtl(GetPathToEtl(L"resources\\AppAnalysis\\UIThreadUtilizationRuleNoFire.etl").c_str());

        VERIFY_IS_FALSE(fired);
    }


} } } } } }
