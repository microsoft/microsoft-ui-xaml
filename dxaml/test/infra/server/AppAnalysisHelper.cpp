// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "NamespaceAliases.h"
#include "AppAnalysisHelper.h"
#include "wil_resource.h"
#include <ppltasks.h>


using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace aa = AppAnalysis::Test;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        wrl::ComPtr<appanalysis::IRule> AppAnalysisHelper::s_rule;
        std::vector<Microsoft::WRL::ComPtr<appanalysis::IRuleTriggeredEventArgs>> AppAnalysisHelper::s_triggeredRuleArgs;
        Event AppAnalysisHelper::s_notificationEvent(L"AppAnalysis");
        String AppAnalysisHelper::s_enabledRule;
        UINT64 AppAnalysisHelper::s_rootArgsHandle = 0;
        bool AppAnalysisHelper::s_deleteTrace = true;  // assume test succeeded
        bool AppAnalysisHelper::s_shouldFailTest = false;
        bool AppAnalysisHelper::s_shouldHaveSourceInfo = true;
        std::unique_ptr<AppAnalysis::Test::RuleTester> AppAnalysisHelper::s_tester;

        void AppAnalysisHelper::EnableRule(_In_ unsigned int processId, _In_z_ const wchar_t* ruleId, const wchar_t* testIdentifier, bool shouldHaveSourceInfo)
        {
            Reset();
            s_shouldHaveSourceInfo = shouldHaveSourceInfo;
            s_tester.reset(new aa::RuleTester());

            s_tester->EnableRule(ruleId);
            s_enabledRule = ruleId;

            RuleNotificationCallback notificationCallback = RuleNotificationCallback([](appanalysis::IRule* rule, appanalysis::IRuleTriggeredEventArgs* args) {

                wil::unique_hstring title;
                rule->get_Title(&title);

                wil::unique_hstring ruleId;
                rule->get_Id(&ruleId);

                LOG_OUTPUT(L"Rule Triggered: %s, %s", WindowsGetStringRawBuffer(ruleId.get(), nullptr), WindowsGetStringRawBuffer(title.get(), nullptr));
                if (AppAnalysisHelper::IsValidEventArgs(rule, args))
                {
                    // this could happen more than once, ensure we only set the rule once
                    if (!s_rule)
                    {
                       s_rule = rule;
                    }

                    s_triggeredRuleArgs.push_back(wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs>(args));
                    s_notificationEvent.Set();
                }
                else
                {
                    LOG_WARNING(L"Invalid notification was fired! See trace at end of test run for further analysis");
                    s_deleteTrace = false; // set this so we dont' delete the trace when we stop the live session
                }
                return S_OK;
            });

            s_tester->SetCallback(notificationCallback);

            s_tester->ProcessLive(processId, testIdentifier);
        }

        void AppAnalysisHelper::VerifyRuleTriggered(_In_ unsigned int expectedTimesTriggered)
        {
            s_notificationEvent.WaitForNoThrow(std::chrono::milliseconds(5000), false /*enforceUnderDebugger*/);

            // stop the live session and let the buffers clear out. at this point we know it's safe to tear things
            // down
            std::wstring etlLog = s_tester->StopLiveSession();

            LOG_OUTPUT(L"Verifying rule fired %d times", expectedTimesTriggered);
            if (expectedTimesTriggered != static_cast<unsigned int>(s_notificationEvent.TimesFired()))
            {
                LOG_ERROR(L"  EXPECTED: %d", expectedTimesTriggered);
                LOG_ERROR(L"  ACTUAL:   %d", s_notificationEvent.TimesFired());
                s_shouldFailTest = true;
                s_deleteTrace = false;
            }

            // if the test was successful, and we did write a file, go ahead and
            // clean it up since we are no longer interested in it
            if (s_deleteTrace && !etlLog.empty())
            {
                ::DeleteFile(etlLog.c_str());
            }
            else if (!s_deleteTrace)
            {
                LOG_OUTPUT(L"See %s for the logs of this test run.", etlLog.c_str());
            }

            Throw::IfFalse(s_notificationEvent.HasFired(), HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            // Fail the test here if we should
            Throw::If(s_shouldFailTest, E_FAIL);

            // this method just verifies that we have metadata for each property. will fail if ever something isn't set
            VerifyRuleProperties(s_rule.Get());
        }

        void AppAnalysisHelper::VerifyMeasurement(_In_ unsigned int index, _In_ unsigned int measurementUnit, _In_ double measurementValue)
        {
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> info = s_triggeredRuleArgs[index];
            appanalysis::Measurement measurement = { 0 };
            LogThrow_IfFailed(info->get_MeasurementUnit(&measurement.Unit));
            LogThrow_IfFailed(info->get_MeasurementValue(&measurement.Value));
            if (static_cast<unsigned int>(measurement.Unit) != measurementUnit && measurementValue != measurement.Value)
            {
                LOG_ERROR(L"Measurement doesn't match:");
                LOG_OUTPUT(L"   EXPECTED: %d %s", measurementValue, ConvertMeasurementUnit(static_cast<appanalysis::MeasurementUnit>(measurementUnit)));
                LOG_OUTPUT(L"   ACTUAL: %d %s", measurement.Value, ConvertMeasurementUnit(measurement.Unit));
                Throw::Exception(E_UNEXPECTED);
            }
        }

        void AppAnalysisHelper::VerifySourceInfo(_In_ unsigned int index, _In_z_ const wchar_t* fileName, _In_ unsigned int lineNumber, _In_ unsigned int columnNumber)
        {
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> info = s_triggeredRuleArgs[index];
            wil::unique_sourceinfo sourceInfo;
            LogThrow_IfFailed(info->get_FileName(&sourceInfo.FileName));
            LogThrow_IfFailed(info->get_LineNumber(&sourceInfo.LineNumber));
            LogThrow_IfFailed(info->get_ColumnNumber(&sourceInfo.ColumnNumber));
            Throw::If(!!WindowsIsStringEmpty(sourceInfo.FileName), E_FAIL, L"SourceInfo.FileName is expected to be returned from notification.");
            int comparison = 0;
            LogThrow_IfFailed(WindowsCompareStringOrdinal(sourceInfo.FileName, StringRef(fileName), &comparison));

            if (comparison != 0 && sourceInfo.LineNumber != lineNumber && sourceInfo.ColumnNumber != columnNumber)
            {
                LOG_ERROR(L"SourceInfo doesn't match:");
                LOG_OUTPUT(L"   EXPECTED: File: %s, Line: %d, Column: %d", fileName, lineNumber, columnNumber);
                LOG_OUTPUT(L"   ACTUAL: File: %s, Line: %d, Column: %d",
                    WindowsGetStringRawBuffer(sourceInfo.FileName, nullptr), sourceInfo.LineNumber, sourceInfo.ColumnNumber);
                Throw::Exception(E_UNEXPECTED);
            };
        }

        void AppAnalysisHelper::DisableCurrentRule()
        {
            Reset();
        }

        void AppAnalysisHelper::Reset()
        {
            s_notificationEvent.Reset();
            s_triggeredRuleArgs.clear();
            s_rule.Reset();
            s_tester.reset();
            s_enabledRule.Empty();
            s_rootArgsHandle = 0;
            s_deleteTrace = true;
            s_shouldFailTest = false;
            s_shouldHaveSourceInfo = true;
        }

        void AppAnalysisHelper::VerifyRuleProperties(_In_ appanalysis::IRule* rule)
        {
            // Verify basic properties are returned from every rule
            wil::unique_hstring ruleId;
            LogThrow_IfFailed(rule->get_Id(&ruleId));
            Throw::IfFalse(s_enabledRule.Compare(WindowsGetStringRawBuffer(ruleId.get(), nullptr)) == 0, E_UNEXPECTED);

            wil::unique_hstring ruleTitle;
            LogThrow_IfFailed(rule->get_Title(&ruleTitle));

            Throw::If(!!WindowsIsStringEmpty(ruleTitle.get()), E_POINTER);

            wil::unique_hstring ruleImpact;
            LogThrow_IfFailed(rule->get_Impact(&ruleImpact));
            Throw::If(!!WindowsIsStringEmpty(ruleImpact.get()), E_POINTER);

            wil::unique_hstring ruleLinkTitle;
            LogThrow_IfFailed(rule->get_LinkTitle(&ruleLinkTitle));
            Throw::If(!!WindowsIsStringEmpty(ruleLinkTitle.get()), E_POINTER);

            wil::unique_hstring ruleLinkUrl;
            LogThrow_IfFailed(rule->get_LinkUri(&ruleLinkUrl));
            Throw::If(!!WindowsIsStringEmpty(ruleLinkUrl.get()), E_POINTER);

        }

        void AppAnalysisHelper::VerifyCanLinkToLVT(_In_ unsigned int index, _In_ UINT64 lvtHandle)
        {
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> info = s_triggeredRuleArgs[index];
            UINT64 elementId = 0;
            LogThrow_IfFailed(info->get_ElementId(&elementId));
            if (elementId != lvtHandle)
            {
                LOG_ERROR(L"Handle values did not match:");
                LOG_OUTPUT(L"EXPECTED: 0x%X,  ACTUAL: 0x%I64X", lvtHandle, elementId);
                Throw::Exception(E_UNEXPECTED);
            }
        }

        void AppAnalysisHelper::VerifyDescription(_In_ unsigned int object, _In_ unsigned int resourceId, _In_ unsigned int count, _In_reads_z_(count) const WCHAR** args)
        {
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> info = s_triggeredRuleArgs[object];
            wrl::ComPtr<appanalysis::IResourceStringView> description;
            LogThrow_IfFailed(info->get_Description(&description));

            UINT actualId = 0;
            LogThrow_IfFailed(description->get_Identifier(&actualId));
            if (actualId != resourceId)
            {
                LOG_ERROR(L"Description ID's didn't match:");
                LOG_OUTPUT(L"EXPECTED: %d,  ACTUAL: %d", resourceId, actualId);
                Throw::Exception(E_UNEXPECTED);
            }

            wrl::ComPtr<wfc::IVectorView<HSTRING>> descriptionStringVector;
            LogThrow_IfFailed(description.As(&descriptionStringVector));

            for (unsigned int i = 0; i < count; ++i)
            {
                int compare = 0;
                wil::unique_hstring actualArg;
                LogThrow_IfFailed(descriptionStringVector->GetAt(i, &actualArg));
                Throw::IfNull(args[i], L"Description argument was null");
                LogThrow_IfFailed(WindowsCompareStringOrdinal(actualArg.get(), StringRef(args[i]), &compare));
                if (compare != 0)
                {
                    LOG_ERROR(L"Description arg #%d didn't match:", i+1);
                    LOG_OUTPUT(L"EXPECTED: %s,  ACTUAL: %s", args[i], WindowsGetStringRawBuffer(actualArg.get(), nullptr));
                    Throw::Exception(E_UNEXPECTED);
                }
            }

        }

        void AppAnalysisHelper::VerifyRuleNotTriggered()
        {
            // We want to WaitForNoThrow because we want to run test scenarios where we make sure we don't report false positives.
            // In this scenario, we would expect not to get a notification so we should only throw an exception from here when something
            // has actually failed.
            s_notificationEvent.WaitForNoThrow(std::chrono::milliseconds(2500));
            Throw::If(s_notificationEvent.HasFired(), E_FAIL);

            std::wstring etlLog = s_tester->StopLiveSession();

            // if the test was successful, and we did write a file, go ahead and
            // clean it up since we are no longer interested in it
            if (s_deleteTrace && !etlLog.empty())
            {
                ::DeleteFile(etlLog.c_str());
            }
            else if (!s_deleteTrace)
            {
                LOG_OUTPUT(L"See %s for the logs of this test run.", etlLog.c_str());
            }
        }

        bool AppAnalysisHelper::IsValidEventArgs(_In_ appanalysis::IRule* rule, _In_ appanalysis::IRuleTriggeredEventArgs* info)
        {
            // We REALLY don't ever want to throw in this method. This happens in a different thread and makes it really hard to understand
            // what's going on from the logs. If any of the APIs fail, we'll mark the test to be failed, and return immediately. This has
            // the benefit of failing the test in VerifyRuleTriggered and makes life easy. Otherwise,  we'll continue to validate the event
            // arguments and log which ones are invalid.

            // if this info object has source info then it's good
            bool isValid = true;
            wil::unique_hstring fileName;
            if (FAILED(info->get_FileName(&fileName)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.FileName");
                s_shouldFailTest = true;
                return false;
            }

            unsigned int lineNumber = 0;
            if (FAILED(info->get_LineNumber(&lineNumber)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.LineNumber");
                s_shouldFailTest = true;
                return false;
            }

            unsigned int columnNumber = 0;
            if (FAILED(info->get_ColumnNumber(&columnNumber)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.ColumnNumber");
                s_shouldFailTest = true;
                return false;
            }

            if (s_shouldHaveSourceInfo && (!!WindowsIsStringEmpty(fileName.get()) || lineNumber == 0 || columnNumber == 0))
            {
                LOG_WARNING(L"Event args didn't have source info! See trace at end of test run for further analysis");
                s_deleteTrace = false; // set this so we dont' delete the trace when we stop the live session
                isValid = false;
            }
            else
            {
                LOG_OUTPUT(L"   RuleTriggeredEventArgs.SourceInfo: %s (%d,%d)",
                    WindowsGetStringRawBuffer(fileName.get(), nullptr), lineNumber, columnNumber);
            }

            appanalysis::Measurement measurement = { 0 };
            if (FAILED(info->get_MeasurementUnit(&measurement.Unit)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.MeasurementUnit");
                s_shouldFailTest = true;
                return false;
            }

            if (FAILED(info->get_MeasurementValue(&measurement.Value)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.MeasurementValue");
                s_shouldFailTest = true;
                return false;
            }

            LOG_OUTPUT(L"   RuleTriggeredEventArgs.Measurement: %4.2f %s", measurement.Value, ConvertMeasurementUnit(measurement.Unit));

            wrl::ComPtr<appanalysis::IResourceStringView> description;
            if (FAILED(info->get_Description(&description)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.Description");
                s_shouldFailTest = true;
                return false;
            }

            if (description == nullptr || !IsValidResourceString(description.Get()))
            {
                // This happens off in some background processing thread so if we throw here,
                // it makes for a very poor debugging experience. We'll log an error and note that we
                // should fail this test and will do so in VerifyRuleTriggered(). We can still delete the
                // .etl file. Not having a description just means the rule was written wrong.
                LOG_ERROR(L"RuleTriggeredEventArgs.Description was null!");
                s_shouldFailTest = true;
            }

            wil::unique_hstring decriptionString;
            if (FAILED(rule->FormatString(description.Get(), &decriptionString)))
            {
                LOG_ERROR(L"Rule.FormatString(description) failed");
                s_shouldFailTest = true;
                return false;
            }

            if (!!WindowsIsStringEmpty(decriptionString.get()))
            {
                // This happens off in some background processing thread so if we throw here,
                // it makes for a very poor debugging experience. We'll log an error and note that we
                // should fail this test and will do so in VerifyRuleTriggered(). We can still delete the
                // .etl file. Not having a description just means the rule was written wrong.
                LOG_ERROR(L"RuleTriggeredEventArgs.Description was empty. This string shouldn't be empty.");
                s_shouldFailTest = true;
            }
            else
            {
                LOG_OUTPUT(L"   RuleTriggeredEventArgs.Description : %s", WindowsGetStringRawBuffer(decriptionString.get(), nullptr));
            }

            wrl::ComPtr<appanalysis::IResourceStringView> solution;
            if (FAILED(info->get_Solution(&solution)))
            {
                LOG_ERROR(L"Failed getting RuleTriggeredEventArgs.Solution");
                s_shouldFailTest = true;
                return false;
            }

            if (solution == nullptr)
            {
                LOG_ERROR(L"RuleTriggeredEventArgs.Solution was null!");
                s_shouldFailTest = true;
            }

            wil::unique_hstring solutionString;
            if (FAILED(rule->FormatString(solution.Get(), &solutionString)))
            {
                LOG_ERROR(L"Rule.FormatString(solution) failed");
                s_shouldFailTest = true;
                return false;
            }

            if (!!WindowsIsStringEmpty(solutionString.get()))
            {
                // Solutions aren't mandatory, but if a solution was provided in the event args and it didn't'
                // return a string, then something went wrong.
                LOG_WARNING(L"RuleTriggeredEventArgs.Solution was empty");
            }

            return isValid;
        }

        PCWSTR AppAnalysisHelper::ConvertMeasurementUnit(_In_ appanalysis::MeasurementUnit unit)
        {
            switch (unit)
            {
            case appanalysis::MeasurementUnit_Kilobytes:
                return L"KB";
            case appanalysis::MeasurementUnit_Milliseconds:
                return L"ms";
            case appanalysis::MeasurementUnit_Elements:
                return L"Elements";
            case appanalysis::MeasurementUnit_Percentage:
                return L"%";
            default:
                return L"unknown";
            }
        }

        bool AppAnalysisHelper::IsValidResourceString(_In_ appanalysis::IResourceStringView* string)
        {
            wrl::ComPtr<wfc::IVectorView<HSTRING>> args;
            UINT32 id = 0;
            LogThrow_IfFailed(string->get_Identifier(&id));
            Throw::If(id == 0, E_INVALIDARG, L"Identifier for resource should not be 0");

            LogThrow_IfFailed(string->QueryInterface<wfc::IVectorView<HSTRING>>(&args));

            UINT32 size = 0;
            LogThrow_IfFailed(args->get_Size(&size));
            for (UINT32 i = 0; i < size; ++i)
            {
                wil::unique_hstring arg;
                LogThrow_IfFailed(args->GetAt(i, &arg));

                // if any of the arguments have a '%n', this will bubble to the user and we don't want that.
                // we'll only compare the first
                PCWSTR stringBuffer = WindowsGetStringRawBuffer(arg.get(), nullptr);
                if (!stringBuffer)
                {
                    LOG_ERROR(L"argument should not be null");
                    return false;
                }
                else if (stringBuffer[0] == L'%' || wcscmp(stringBuffer, L"UnknownType") == 0)
                {
                    LOG_ERROR(L"Argument: %s is invalid", stringBuffer);
                    return false;
                }
            }

            return true;
        }
    }
    } } } }
