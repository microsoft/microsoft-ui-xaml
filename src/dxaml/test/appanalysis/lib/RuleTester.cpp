// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TraceSession.h"
#include "RuleTester.h"
#include "Pathcch.h"
#include "wexexception.h"
#include <unordered_set>
#include <ppltasks.h>
#include "wil_resource.h"
#include "TestRuleSet.h"
#include "log.h"

const GUID c_sessionGuid = { 0xc5182871, 0x127a, 0x48a0 ,{ 0x8a, 0x30, 0x3b, 0x88, 0x95, 0x79, 0xe4, 0x5b } };
const GUID c_xamlDiagnosticGuid = { 0x59e7a714, 0x73a4, 0x4147,{ 0xb4, 0x7e, 0x09, 0x57, 0x04, 0x8c, 0x75, 0xc4 } };
const GUID c_xamlGuid = { 0x531a35ab, 0x63ce, 0x4bcf,{ 0xaa, 0x98, 0xf8, 0x8c, 0x7a, 0x89, 0xe4, 0x55 } };

#define LOG_WARNING(fmt, ...) WEX::Logging::Log::Warning(WEX::Common::String().Format(fmt, __VA_ARGS__))
#define LOG_OUTPUT(fmt, ...) WEX::Logging::Log::Comment(WEX::Common::String().Format(fmt, __VA_ARGS__))

using namespace WEX::Common;

const GUID c_providerGuids[] = {
    c_xamlGuid,
    c_xamlDiagnosticGuid
};

namespace AppAnalysis { namespace Test {
    #define InterlockedExchangeState(target, value) \
    static_cast<TraceState>(InterlockedExchange((UINT*)target, static_cast<UINT>(value))); \
    \

    #define InterlockedCompareExchangeState(target, value, compared) \
    static_cast<TraceState>(InterlockedCompareExchange((UINT*)target, static_cast<UINT>(value), static_cast<UINT>(compared))); \
    \

volatile TraceState RuleTester::s_traceState = TraceState::Invalid;

bool RuleTester::IsShuttingDown()
{
    return s_traceState == TraceState::ShuttingDown || s_traceState == TraceState::Invalid;
}

RuleTester::RuleTester()
    : m_ruleSetType(RuleSetType::None)
    , m_processingLive(false)
    , m_enabledRuleToken({0})
{
    LoadModule(L"AppAnalysis\\Microsoft.Diagnostics.AppAnalysis.dll", &m_appAnalysisModule);

    GetClassObject = (pfnGetClassObject)(GetProcAddress(m_appAnalysisModule.get(), "DllGetActivationFactory"));
    if (!GetClassObject)
    {
        Throw::LastError();
    }

    ProcessEtwEvent = (pfnProcessEvent)(GetProcAddress(m_appAnalysisModule.get(), "ProcessEvent"));

    Throw::IfFailed(wf::Initialize(RO_INIT_MULTITHREADED));
    m_mutex.create();
}

RuleTester::~RuleTester()
{
    Shutdown();

    wf::Uninitialize();
}

std::wstring RuleTester::StopLiveSession()
{
    // Set the shutdown event and wait for the consumer to finish processing.
    ASSERT(m_processingLive);
    TraceState previousState = InterlockedCompareExchangeState(&s_traceState, TraceState::ShuttingDown, TraceState::ProcessingEvents);
    std::wstring fileName;
    std::wstring mergedEtlName;
    if (m_traceSession && previousState == TraceState::ProcessingEvents)
    {
        unsigned int lostEvents = 0;
        Throw::IfFailed(m_traceSession->GetTraceDetails(&fileName, &lostEvents));

        // remove the Unmerged_ from the file name
        mergedEtlName = fileName.substr(fileName.find_first_of(L"_") + 1);
        if (lostEvents > 0)
        {
            LOG_WARNING(L"%d events were lost, results may be innaccurate", lostEvents);
        }
        
        Throw::IfFailed(m_traceSession->Shutdown(), L"TraceSession.Shutdown failed");
        // Wait until shutdown is complete before releasing the trace session
        if (m_shutdownCompleteEvent)
        {
            m_shutdownCompleteEvent.wait();
            m_shutdownCompleteEvent.ResetEvent();
        }

        DWORD status = TraceSession::MergeEtl(fileName, mergedEtlName);
        if (status == ERROR_INSUFFICIENT_BUFFER)
        {
            LOG_WARNING(L"Events were lost merging the log file");
        }
        else
        {
            Throw::IfFailed(HRESULT_FROM_WIN32(status), L"Failed to merge the etl file");
        }

        m_traceSession.reset();
    }

    return mergedEtlName;
}

void RuleTester::Shutdown()
{
    // pass in false, assume the test failed if StopLiveSession wasn't called externally
    if (m_processingLive)
    {
        StopLiveSession();
    }

    if (m_enabledRule)
    {
        if (m_enabledRuleToken.value > 0)
        {
            wrl::ComPtr<appanalysis::IRule> backingRule;
            Throw::IfFailed(m_enabledRule->get_BackingRule(&backingRule));
            backingRule->remove_Triggered(m_enabledRuleToken);
        }
        Throw::IfFailed(m_enabledRule->Stop(), L"EtwRule.Stop failed");
    }
    
    m_enabledRule.Reset();
    m_ruleSet.Reset();
    m_traceSession.reset();

    m_appAnalysisModule.reset();
    InterlockedExchangeState(&s_traceState, TraceState::Invalid);

}

void RuleTester::LoadModule(
    _In_z_ const wchar_t * pathToLibrary, 
    _Out_ HMODULE * module
    )
{
    *module = nullptr;
    wchar_t currentDirectory[MAX_PATH] = { 0 };
    DWORD written = GetCurrentDirectory(MAX_PATH, currentDirectory);
    if (written == 0)
    {
        Throw::LastError();
    }
    LOG_OUTPUT(L"Current directory: %s", currentDirectory);
    LOG_OUTPUT(L"Loading %s", pathToLibrary);
    HMODULE loadedModule = LoadLibrary(pathToLibrary);
    if (!loadedModule)
    {
        // if null here then the tests are being run locally and we retry to load
        wchar_t finalPath[MAX_PATH] = { 0 };
        Throw::IfFailed(PathCchCombineEx(finalPath, MAX_PATH, L"\\xcp\\", pathToLibrary, 0));
        wchar_t finalPath2[MAX_PATH] = { L".." };
        Throw::IfFailed(StringCchCatEx(finalPath2, MAX_PATH, finalPath,nullptr, nullptr, 0));

        LOG_OUTPUT(L"Previous attempt failed, loading %s", finalPath2);
        loadedModule = LoadLibrary(finalPath2);
        if (!loadedModule)
        {
            // if loadedModule is still null then there is an issue
            Throw::LastError();
        }
    }

    *module = loadedModule;
}

void RuleTester::EnableRuleInternal(
    _In_z_ PCWSTR stringId
    )
{
    // Lock the mutex at this point, if currently processing a rule,
    // we will block;
    auto lock = m_mutex.acquire();

    // Start out assuming they passed in invalid rule ID.
    HRESULT hr = E_INVALIDARG;

    UINT count = 0;
    Throw::IfFailed(m_ruleSet->get_Size(&count));
    for (UINT i = 0; i < count; i++)
    {
        wrl::ComPtr<appanalysis::IRule> rule;
        wrl::ComPtr<appanalysis::IEtwRule> etwRule;
        Throw::IfFailed(m_ruleSet->GetAt(i, &etwRule));
        
        Throw::IfFailed(etwRule->get_BackingRule(&rule), L"EtwRule.BackingRule failed");
        Throw::IfNull(rule.Get(), L"EtwRule.BackingRule is null");

        wil::unique_hstring ruleId;
        Throw::IfFailed(rule->get_Id(&ruleId));

        int compare = 0;
        Throw::IfFailed(WindowsCompareStringOrdinal(StringRef(stringId), ruleId.get(), & compare));
        if (compare == 0)
        {
            m_enabledRule = etwRule;
            wil::unique_hstring title;
            rule->get_Title(&title);

            LOG_OUTPUT(L"Rule Enabled: %s, %s", WindowsGetStringRawBuffer(ruleId.get(), nullptr), WindowsGetStringRawBuffer(title.get(), nullptr));
            Throw::IfFailed(m_enabledRule->Start(), L"EtwRule.Start failed");

            InterlockedExchangeState(&s_traceState, TraceState::RuleEnabled);
            hr = S_OK;
            break;
        }
    }

    Throw::IfFailed(hr);
}

void RuleTester::EnsureRuleSet(
    _Outptr_ RuleSet** ruleSet)
{
    WEX::Common::Throw::IfNull(ruleSet);
    WEX::Common::Throw::If(m_ruleSetType != RuleSetType::None && RuleSetType::SDK != m_ruleSetType, E_ILLEGAL_STATE_CHANGE,
        L"This rule tester can not be re-used to test a rule from a different rule set");

    if (*ruleSet == nullptr)
    {
        wrl::ComPtr<IActivationFactory> ruleSetFactory;
        Throw::IfFailed(GetClassObject(StringRef(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRuleSet), &ruleSetFactory));
        wrl::ComPtr<IInspectable> setAsInsp;
        Throw::IfFailed(ruleSetFactory->ActivateInstance(&setAsInsp), std::wstring(L"Failed to create ").append(RuntimeClass_Microsoft_Diagnostics_AppAnalysis_EtwRuleSet).c_str());
        Throw::IfFailed(setAsInsp.CopyTo(ruleSet));
    }
  
    m_ruleSetType = RuleSetType::SDK;
}

void RuleTester::EnsureTestRuleSet(
    _Outptr_ RuleSet** ruleSet)
{
    WEX::Common::Throw::IfNull(ruleSet);
    WEX::Common::Throw::If(m_ruleSetType != RuleSetType::None && RuleSetType::Test != m_ruleSetType, E_ILLEGAL_STATE_CHANGE,
        L"This rule tester can not be re-used to test a rule from a different rule set");

    if (*ruleSet == nullptr)
    {
        LoadModule(L"AppAnalysis\\AppAnalysis.Test.TestRuleSet.dll", &m_testRulesModule);
        pfnGetClassObject GetTestClassObject = (pfnGetClassObject)(GetProcAddress(m_testRulesModule.get(), "DllGetActivationFactory"));

        wrl::ComPtr<IActivationFactory> ruleSetFactory;
        Throw::IfFailed(GetTestClassObject(StringRef(L"AppAnalysis.Test.TestRuleSet"),&ruleSetFactory));
        wrl::ComPtr<IInspectable> setAsInsp;
        Throw::IfFailed(ruleSetFactory->ActivateInstance(&setAsInsp), L"Failed to create AppAnalysis.Test.TestRuleSet");
        Throw::IfFailed(setAsInsp.CopyTo(ruleSet));
    }

    m_ruleSetType = RuleSetType::Test;
}

EVENT_RECORD RuleTester::CreateEventRecord(LONGLONG time, GUID provider, USHORT id, UCHAR version)
{
    EVENT_RECORD eventRecord = { 0 };
    eventRecord.EventHeader.TimeStamp.QuadPart = time;
    eventRecord.EventHeader.ProviderId = provider;
    eventRecord.EventHeader.EventDescriptor.Id = id;
    eventRecord.EventHeader.EventDescriptor.Version = version;
    eventRecord.EventHeader.Size = sizeof(EVENT_RECORD);

    return eventRecord;
}

/**
    EnableTestRule enables the RuleTester to test a rule that is contained in the TestRuleSet by first creating
    a TestRuleSet object then calling EnableRuleInternal to perform code that is common between both test rules and 
    real engine rules.
*/
void 
RuleTester::EnableTestRule(
    _In_z_ PCWSTR testRuleId
    )
{
    EnsureTestRuleSet(&m_ruleSet);
    EnableRuleInternal(testRuleId);
}

/**
    EnableRule enables the RuleTester to test a rule that is contained in the AppAnalysis engine itself by first creating
    a RuleSet object then calling EnableRuleInternal to perform code that is common between both test rules and
    real engine rules.
*/
void
RuleTester::EnableRule(
    _In_z_ PCWSTR ruleId
    )
{
    EnsureRuleSet(&m_ruleSet);
    EnableRuleInternal(ruleId);
}

void 
RuleTester::ProcessEtl(
    _In_z_ PCWSTR pathToETL
    )
{
    // callback must be set before calling ProcessEtl
    ASSERT(m_enabledRuleToken.value > 0);

    Throw::IfFailed(TraceSession::CreateInstance(&m_traceSession));

    Throw::IfFailed(m_traceSession->StartEtlTraceSession(pathToETL));

    // Process Trace will block until the session is stopped or
    // all events in the etl were processed.
    InterlockedExchangeState(&s_traceState, TraceState::ProcessingEvents);
    Throw::IfFailed(m_traceSession->ProcessTrace());
}

void
RuleTester::SetCallback(
    _In_ const RuleNotificationCallback& notificationCallback
    )
{
    auto callback = wrl::Callback<wf::ITypedEventHandler<appanalysis::IRule*, appanalysis::RuleTriggeredEventArgs*>>([notificationCallback](appanalysis::IRule* rule, appanalysis::IRuleTriggeredEventArgs* args)
    {
        IFC_RETURN(notificationCallback(rule, args));
        return S_OK;
    });

    wrl::ComPtr<appanalysis::IRule> rule;
    Throw::IfFailed(m_enabledRule->get_BackingRule(&rule), L"EtwRule.BackingRule failed");
    Throw::IfFailed(rule->add_Triggered(callback.Get(), &m_enabledRuleToken), L"Failed to register for the Triggered event");
}

void
RuleTester::ProcessLive(
    _In_ UINT processId,
    _In_z_ PCWSTR testIdentifier
    )
{
    // callback must be set before calling ProcessLive
    auto lock = m_mutex.acquire();
    ASSERT(m_enabledRuleToken.value > 0);
    ASSERT(!m_traceSession);

    Throw::IfFailed(TraceSession::CreateInstance(&m_traceSession));

    // the test identifier will become the session name as well as what is used to write the etl logs.
    // the idea is that we can have a log called AppAnalysis_VerifyImageDecodingRule" where "VerifyImageDecodingRule"
    // is the identifier.
    std::wstring sessionName = std::wstring(L"AppAnalysis_").append(testIdentifier);
    Throw::IfFailed(m_traceSession->StartLiveTraceSession(sessionName, processId, c_sessionGuid), L"Failed to start the trace session");

    m_processingLive = true;

    for (auto& guid : c_providerGuids)
    {
        Throw::IfFailed(m_traceSession->EnableProvider(guid));
    }

    wil::unique_event processThreadBegun;
    processThreadBegun.create(wil::EventOptions::ManualReset);

    m_shutdownCompleteEvent.create(wil::EventOptions::ManualReset);

    Concurrency::create_task([&]() {
        processThreadBegun.SetEvent();
        InterlockedExchangeState(&s_traceState, TraceState::ProcessingEvents);
        HRESULT hr = m_traceSession->ProcessTrace();
        m_shutdownCompleteEvent.SetEvent();
        WEX::Common::Throw::IfFailed(hr, L"TraceSession.ProcessTrace failed");
    });

    // wait for the process thread to begin, otherwise we run into scenarios where tests
    // complete immediately and fail since we haven't started processing.
    processThreadBegun.wait();
}

void 
RuleTester::ProcessMockEvent(
    _In_ const MockEtwEvent& etwEvent)
{
    // callback must be set before calling ProcessMockEvent
    ASSERT(m_enabledRuleToken.value > 0);

    EVENT_RECORD eventRecord = CreateEventRecord(etwEvent.Time, etwEvent.Provider, etwEvent.Id, etwEvent.Version);
    ProcessEvent(&eventRecord);
}

void RuleTester::ProcessEvent(
    _In_ PEVENT_RECORD eventRecord)
{
    InterlockedExchangeState(&s_traceState, TraceState::ProcessingEvents);
    Throw::IfFailed(ProcessEtwEvent(eventRecord), L"EventProcessor.ProcessEvent failed");
}

Microsoft::WRL::ComPtr<RuleTester::RuleSet>
RuleTester::GetRules(
    _In_ bool getFromTestRuleSet
)
{
    if (getFromTestRuleSet)
    {
        EnsureTestRuleSet(&m_ruleSet);
    }
    else
    {
        EnsureRuleSet(&m_ruleSet);
    }

    return m_ruleSet;
}

} }
