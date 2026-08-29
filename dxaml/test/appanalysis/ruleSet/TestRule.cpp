// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "TestEtwRuleImpl.h"
#include "helpers.h"

////////////////////////////////////////////////////////////////////////////////
// The TestRule is to be created by the TestRuleSet. Use the MockRule object if 
// you want a standalone rule for testing functionality outside of a RuleSet
//
using namespace Microsoft::Diagnostics::AppAnalysis;
class TestRule
    : public TestEtwRuleImpl<TestRule, appanalysis::RuleCategories_Performance>
{

public:
    ////////////////////////////////////////////////////////////////////////////////
    //
    TestRule()
        : m_llInitializeCoreBeginTimeStamp(0)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    virtual ~TestRule()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    HRESULT TestRule::ProcessInitializeCoreBeginEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        HRESULT hr = S_OK;

        ASSERT(m_llInitializeCoreBeginTimeStamp == 0);
        IFC(pEvent->get_Timestamp(&m_llInitializeCoreBeginTimeStamp));

    Cleanup:
        return hr;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    HRESULT TestRule::ProcessApplicationStartupInfoEvent(
        _In_ appanalysis::IEtwEventRecord* pEvent
        )
    {
        HRESULT hr = S_OK;
        LONGLONG delta, timeStamp;

        ASSERT(m_llInitializeCoreBeginTimeStamp != 0);
        IFC(pEvent->get_Timestamp(&timeStamp));
        delta = HNS_TO_MS(timeStamp - m_llInitializeCoreBeginTimeStamp);

        // Assume a 0.2 second threshold for app startup.
        if (delta > 200)
        {
            wrl::ComPtr<appanalysis::IRuleTriggeredEventArgs> spNotificationInfo;
            appanalysis_impl::RuleTriggeredEventArgs::CreateParams params;

            params.timeline = {
                timeStamp,
                timeStamp
            };

            params.measurement = {
                static_cast<DOUBLE>(delta),
                appanalysis::MeasurementUnit_Milliseconds
            };

            IFC_RETURN(wrl::MakeAndInitialize<appanalysis_impl::ResourceString>(&params.description, TEST_RULE_DESCRIPTION));
            IFC_RETURN(wrl::MakeAndInitialize<appanalysis_impl::ResourceString>(&params.solution, TEST_RULE_SOLUTION));

            IFC(CreateRuleTriggeredEventArgs(params, &spNotificationInfo));
            FireNotification(spNotificationInfo.Get());
        }


        // Reset
        m_llInitializeCoreBeginTimeStamp = 0;

    Cleanup:
        return hr;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        HRESULT TestRule::ProcessParseXAMLBeginEvent(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            UNREFERENCED_PARAMETER(pEvent);
            return S_OK;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        HRESULT TestRule::ProcessMeasureElementBeginEvent(
            _In_ appanalysis::IEtwEventRecord* pEvent
            )
        {
            UNREFERENCED_PARAMETER(pEvent);
            return S_OK;
        }

private:
    LONGLONG m_llInitializeCoreBeginTimeStamp;
};

BEGIN_PROVIDERS(TestRule)
    DECLARE_MANIFEST_PROVIDER(WINDOWS_UI_XAML_ETW_PROVIDER, L"Microsoft-Windows-XAML-ETW.man")
END_PROVIDERS()

BEGIN_CALLBACKS(TestRule)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ApplicationStartupInfo_value, EventVersion_0, &TestRule::ProcessApplicationStartupInfoEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, InitializeCoreBegin_value, EventVersion_0, &TestRule::ProcessInitializeCoreBeginEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, ParseXamlBegin_value, EventVersion_0, &TestRule::ProcessParseXAMLBeginEvent)
    DECLARE_EVENT_CALLBACK(WINDOWS_UI_XAML_ETW_PROVIDER, MeasureElementBegin_value, EventVersion_0, &TestRule::ProcessMeasureElementBeginEvent)
END_CALLBACKS()

namespace AppAnalysis { namespace Test {
////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TestRule_CreateInstance(
     _COM_Outptr_ appanalysis::IEtwRule** ppInstance
    )
{
    // Initialize the output parameter to nullptr at the start
    *ppInstance = nullptr;
    wrl::ComPtr<TestRule> rule;
    IFC_RETURN(TestRule::CreateInstance(
        L"Rule001", TEST_RULE_TITLE,
        TEST_RULE_IMPACT, TEST_RULE_LINK_TITLE, L"http://testrule.com",
        &rule));

    wrl::ComPtr<appanalysis::IEtwEventWatcher> watcher;
    IFC_RETURN(rule->RegisterTestEvents(&watcher));

    IFC_RETURN(rule->CreateEtwRule(ppInstance, watcher.Get()));

    return S_OK;
}
} }
