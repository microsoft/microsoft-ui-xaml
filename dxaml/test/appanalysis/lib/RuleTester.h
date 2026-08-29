// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "wil\resource.h"
#include "wil_resource.h"
#include <functional>
#include <ppltasks.h>
#include "evntrace.h"

using RuleNotificationCallback = std::function<HRESULT(appanalysis::IRule*, appanalysis::IRuleTriggeredEventArgs*)>;
typedef HRESULT (*pfnGetClassObject)(HSTRING, IActivationFactory**);
typedef HRESULT(*pfnProcessEvent)(PEVENT_RECORD);


// Forward declare for test infra
class TraceSession;

namespace AppAnalysis { namespace Test {

    struct MockEtwEvent {
        GUID Provider;
        UINT16 Id;
        BYTE Version;
        ULONGLONG Time;
    };

    enum TraceState : UINT
    {
        Invalid = 0,
        RuleEnabled = 1,
        TraceStarted = 2,
        ProcessingEvents = 3,
        ShuttingDown = 4,
    };

class RuleTester
{

public:
    using RuleSet = wfc::IVectorView<appanalysis::EtwRule*>;

    RuleTester();
    ~RuleTester();

    static bool IsShuttingDown();

    void EnableTestRule(
        _In_z_ PCWSTR ruleId
        );

    void EnableRule(
        _In_z_ PCWSTR ruleId
        ) ;

    void SetCallback(
        _In_ const RuleNotificationCallback& notificationCallback
        );

    void ProcessEtl(
        _In_z_ PCWSTR pathToETL
        );

    void ProcessLive(
        _In_ UINT processId,
        _In_z_ PCWSTR testIdentifier
        );

    void ProcessMockEvent(
        _In_ const MockEtwEvent& etwEvent
        );

    // Returns the path to the .etl
    std::wstring StopLiveSession(
        );

    Microsoft::WRL::ComPtr<RuleSet> GetRules(
        _In_ bool getFromTestRuleSet = false
    );

private:

    RuleTester(const RuleTester& other) = delete;
    RuleTester& operator=(const RuleTester& other) = delete;

    void EnableRuleInternal(
        _In_z_ PCWSTR ruleId
        );

    void EnsureRuleSet(
        _Outptr_ RuleSet** ruleSet
        );

    void EnsureTestRuleSet(
        _Outptr_ RuleSet** ruleSet
        );

    void ProcessEvent(
        _In_ PEVENT_RECORD eventRecord
        );

    void Shutdown();

    static EVENT_RECORD CreateEventRecord(LONGLONG time, GUID provider, USHORT id, UCHAR version);
    static void LoadModule(_In_z_ const wchar_t* pathToModule, _Out_ HMODULE* module);

    Microsoft::WRL::ComPtr<RuleSet> m_ruleSet;
    std::unique_ptr<TraceSession> m_traceSession;
    Microsoft::WRL::ComPtr<appanalysis::IEtwRule> m_enabledRule;
    EventRegistrationToken m_enabledRuleToken;
    pfnGetClassObject GetClassObject;
    pfnProcessEvent ProcessEtwEvent;

    wil::unique_hmodule m_appAnalysisModule;
    wil::unique_hmodule m_testRulesModule;
    wil::unique_event m_shutdownCompleteEvent;

    enum RuleSetType {
        None = 0,
        SDK,
        Test
    };

    RuleSetType m_ruleSetType;

    static volatile TraceState s_traceState;

    wil::unique_mutex m_mutex;

    bool m_processingLive;
};
   
}}
