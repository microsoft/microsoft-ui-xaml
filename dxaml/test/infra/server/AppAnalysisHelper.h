// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "microsoft.diagnostics.appanalysis.internal.h"
#include "ruletester.h"
#include <chrono>
#include <TestEvent.h>
#include <map>
#include "wil\resource.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class AppAnalysisHelper
        {
        public:
            static void EnableRule(_In_ unsigned int processId, _In_z_ const wchar_t* ruleId, const wchar_t* testIdentifier, bool shouldHaveSourceInfo);
            static void VerifyRuleTriggered(_In_ unsigned int expectedTimesTriggered);
            static void VerifyRuleNotTriggered();
            static void VerifyMeasurement(_In_ unsigned int object, _In_ unsigned int measurementUnit, _In_ double measurementValue);
            static void VerifySourceInfo(_In_ unsigned int object, _In_z_ const wchar_t* fileName, _In_ unsigned int lineNumber, _In_ unsigned int columnNumber);
            static void VerifyCanLinkToLVT(_In_ unsigned int object, _In_ unsigned long long lvtHandle);
            static void VerifyDescription(_In_ unsigned int object, _In_ unsigned int resourceId, _In_ unsigned int count, _In_reads_z_(count) const WCHAR** args);
            static void DisableCurrentRule();

        private:
            static std::unique_ptr<AppAnalysis::Test::RuleTester> s_tester;
            static Microsoft::WRL::ComPtr<appanalysis::IRule> s_rule;
            static std::vector<Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>> s_triggeredRuleArgs;
            static UINT64 s_rootArgsHandle;
            static Event s_notificationEvent;
            static WEX::Common::String s_enabledRule;
            static bool s_deleteTrace;
            static bool s_shouldFailTest;
            static bool s_shouldHaveSourceInfo;

            static void VerifyRuleProperties(_In_ appanalysis::IRule* rule);
            static void VerifyTriggeredCount(_In_ unsigned int count);
            static bool IsValidEventArgs(_In_ appanalysis::IRule* rule,
                                         _In_ appanalysis::IRuleTriggeredEventArgs* info);
            static bool IsValidResourceString(_In_ appanalysis::IResourceStringView* string);
            static PCWSTR ConvertMeasurementUnit(_In_ appanalysis::MeasurementUnit unit);
            static void Reset();
        };
    }
} } } }
