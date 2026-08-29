// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "Module.h"
#include <string>
#include <iostream>
#include <type_traits>
#include <vector>
#include "wil\resource.h"

#define MAX_PID_STRING_LENGTH 10

enum class AppAnalysisCommand
{
    Invalid = 0,
    DisplayUsage,
    Start,
    Run,
    Stop,
};

struct AppAnalysisParameters
{
    AppAnalysisCommand command = AppAnalysisCommand::Invalid;
    bool waitForDebugger = false;
    std::wstring customRuleSet;
    std::wstring processOrFileName;
    std::wstring invalidFlag;
};

HRESULT Main(
    _In_ int argc,
    _In_reads_z_(argc) PCWSTR argv[]
    );

HRESULT RunImpl(
    _In_ const std::wstring& customRuleSet,
    _In_ const std::wstring& processOrFileName,
    _In_ bool waitForDebugger
    );

HRESULT StartImpl(
    _In_ const std::wstring& processName,
    _In_ const std::wstring& customRuleSet,
    _In_ const std::wstring& processOrFileName,
    _In_ bool waitForDebugger
    );

HRESULT StopImpl(
    _In_ const std::wstring& processName
    );

void DisplayUsageImpl(
    const std::wstring& invalidFlag,
    const std::wstring& processName
);

AppAnalysisParameters ParseParameters(
    _In_ int argc, _In_reads_z_(argc) PCWSTR argv[]
    );

bool IsValidExecutableName(
    _In_z_ const wchar_t* string
    );

bool IsValidProcessId(
    _In_z_ const wchar_t* string
    );

bool IsValidEtlName(
    _In_z_ const wchar_t* string
    );

CComModule _Module;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_Module;

const wchar_t* c_drXamlEventName = L"StopDrXamlTracePrivate";

#ifndef QUOTE__
#define TO_STRING__(s) L#s
#define QUOTE__(s) TO_STRING__(s)
#endif
//////////////////////////////////////////////////////////////////////////////
//
int _cdecl
wmain(
    _In_ int argc,
    _In_reads_z_(argc) PCWSTR argv[]
    )
{
    HRESULT hr = S_OK;

    IFC(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    IFC(Main(argc, argv));

Cleanup:
    CoUninitialize();
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL WINAPI CtrlC(
    _In_  DWORD /*dwCtrlType*/
)
{
    const wchar_t * c_exeName = QUOTE__(DR_XAML_TOOL_NAME__);

    return SUCCEEDED(StopImpl(c_exeName));
}

//////////////////////////////////////////////////////////////////////////////
//
HRESULT Main(_In_ int argc, _In_reads_z_(argc) PCWSTR argv[])
{
    wprintf(L"\n");

    AppAnalysisParameters params = ParseParameters(argc, argv);

    HRESULT hr = E_INVALIDARG;

    switch (params.command)
    {
    case AppAnalysisCommand::DisplayUsage:
        hr = S_OK;
    case AppAnalysisCommand::Invalid:
        DisplayUsageImpl(params.invalidFlag, argv[0]);
        return hr;
    case AppAnalysisCommand::Run:
        return RunImpl(params.customRuleSet, params.processOrFileName, params.waitForDebugger);
    case AppAnalysisCommand::Start:
        return StartImpl(argv[0], params.customRuleSet, params.processOrFileName, params.waitForDebugger);
    case AppAnalysisCommand::Stop:
        return StopImpl(argv[0]);
    default:
        ASSERT(FALSE);
        return E_INVALIDARG;
    }
}

AppAnalysisParameters ParseParameters(_In_ int argc, _In_reads_z_(argc) PCWSTR argv[])
{
    AppAnalysisParameters params;

    // skip first arg since that is going to be the name of this executable
    int i = 1;
    for (; i < argc; i++)
    {
        if ((_wcsicmp(argv[i], L"-?") == 0) || (_wcsicmp(argv[i], L"/?") == 0) ||
            (_wcsicmp(argv[i], L"-h") == 0) || (_wcsicmp(argv[i], L"/h") == 0))
        {
            params.command = AppAnalysisCommand::DisplayUsage;
            return params;
        }
#if DBG
        else if ((_wcsicmp(argv[i], L"-d") == 0) || (_wcsicmp(argv[i], L"/d") == 0))
        {
            params.waitForDebugger = true;
        }
#endif
        else if ((_wcsicmp(argv[i], L"-start") == 0) || (_wcsicmp(argv[i], L"/start") == 0))
        {
            params.command = AppAnalysisCommand::Start;
        }
        else if ((_wcsicmp(argv[i], L"-stop") == 0) || (_wcsicmp(argv[i], L"/stop") == 0))
        {
            params.command = AppAnalysisCommand::Stop;
            return params;
        }
        else if ((_wcsicmp(argv[i], L"-run") == 0) || (_wcsicmp(argv[i], L"/run") == 0))
        {
            params.command = AppAnalysisCommand::Run;
        }
        else if ((_wcsicmp(argv[i], L"-ruleSet") == 0) || (_wcsicmp(argv[i], L"/ruleSet") == 0) && (i + 1 <= argc))
        {
            params.customRuleSet = argv[i + 1];
        }
        else if (IsValidExecutableName(argv[i]) || IsValidProcessId(argv[i]) || IsValidEtlName(argv[i]))
        {
            params.processOrFileName = argv[i];
        }
        else
        {
            params.invalidFlag = argv[i];
            return params;
        }
    }

    return params;
}

void DisplayUsageImpl(const std::wstring& invalidFlag, const std::wstring& processName)
{
    if (!invalidFlag.empty())
    {
        wprintf_s(L"Parameter '%s' is invalid flag, executable name, process id, or file name. Please see usage below.\n", invalidFlag.c_str());
    }

#if DBG
    wprintf(L"Usage: %s [-start processName.exe/pid/filename.etl] | [-stop]\n", processName.c_str());
    wprintf(L"       OPTIONAL: [-d] [-? ] [-ruleSet CustomRuleSet]\n");
#else
    wprintf_s(L"Usage: %s [-start processName.exe/pid/filename.etl] | [stop] [-?]\n", processName.c_str());
    wprintf(L"         OPTIONAL: [-? ] [-ruleSet CustomRuleSet]\n");
#endif
    wprintf(L"\n");
    wprintf(L"  -start                              : Process a live trace or trace from etl file.\n");
    wprintf(L"  -stop                               : Stops the session started by %s -start\n", processName.c_str());
#if DBG
    wprintf(L"  -d                                  : Waits for debugger to attach to trace process\n");
#endif
    wprintf(L"  -ruleSet CustomRuleSet              : Include runtime class 'CustomRuleSet' from CustomRuleSet.dll in the trace session. CustomRuleSet.dll needs to be in the path \n");
    wprintf(L"  -?                                  : Displays usage\n");
    wprintf(L"\n");
}

HRESULT StartImpl(
    _In_ const std::wstring& processName,
    _In_ const std::wstring& customRuleSet,
    _In_ const std::wstring& processOrFileName,
    _In_ bool waitForDebugger
)
{
    // Make sure the event can't be opened, this would get created inside of Run. If we can open it here,
    // it means that there is another process lingering.
    wil::unique_event processingFinishedEvent;
    if (processingFinishedEvent.try_open(c_drXamlEventName, SYNCHRONIZE))
    {
        wprintf(L"Already existing instance of %s, you must stop instance before starting a new one.\n", processName.c_str());
        return E_INVALIDARG;
    }

    // if this is an etl file, just directly call RunImpl, since we don't need to create the second process
    if (IsValidEtlName(processOrFileName.c_str()))
    {
        return RunImpl(customRuleSet, processOrFileName, waitForDebugger);
    }

    // build the command with the arguments passed in
    std::wstring finalCommand = processName + L" -run ";
    finalCommand.append(processOrFileName);
    if (!customRuleSet.empty())
    {
        finalCommand.append(L" -ruleSet " + customRuleSet);
    }

    if (waitForDebugger)
    {
        finalCommand.append(L" -d");
    }

    PROCESS_INFORMATION procInfo = { 0 };

    STARTUPINFO startInfo = { 0 };
    GetStartupInfo(&startInfo);

    BOOL success = CreateProcess(
        processName.c_str(),        // process name
        const_cast<LPWSTR>(finalCommand.c_str()),     // command line
        NULL,          // primary thread security attributes
        NULL,
        TRUE,          // handles are inherited
        DETACHED_PROCESS,             // creation flags
        NULL,          // use parent's environment
        NULL,
        &startInfo,  // STARTUPINFO pointer
        &procInfo);  // receives PROCESS_INFORMATION

    // We don't care about these handles anymore
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    if (success == FALSE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    wprintf(L"Processing Live...\n");
    return S_OK;
}

HRESULT RunImpl(
    _In_ const std::wstring& customRuleSet,
    _In_ const std::wstring& processOrFileName,
    _In_ bool waitForDebugger
)
{
#if DBG
    if (waitForDebugger)
    {
        while (!IsDebuggerPresent())
        {
            Sleep(1000);
        }
        DebugBreak();
    }
#else
    UNREFERENCED_PARAMETER(waitForDebugger);
#endif

    // Register for Ctrl+C signals
    IFCW32_RETURN(SetConsoleCtrlHandler(&CtrlC, TRUE));
    auto unregisterHandler = wil::scope_exit([&]{ SetConsoleCtrlHandler(&CtrlC, FALSE); });

    // Load app analysis after user has had chance to break into debugger
    wil::unique_hmodule AppAnalysisDll(LoadLibrary(L"Microsoft.Diagnostics.AppAnalysis.dll"));
    if (!AppAnalysisDll)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    wil::unique_event processingFinishedEvent;
    processingFinishedEvent.create(wil::EventOptions::ManualReset, c_drXamlEventName);

    AppModule appModule;
    IFC_RETURN(appModule.Initialize(customRuleSet));

    if (IsValidExecutableName(processOrFileName.c_str()))
    {
        IFC_RETURN(appModule.ProcessLive(processOrFileName));
    }
    else if (IsValidProcessId(processOrFileName.c_str()))
    {
        DWORD processId = wcstoul(processOrFileName.c_str(), NULL, 10);
        IFC_RETURN(appModule.ProcessLive(processId));
    }
    else if (IsValidEtlName(processOrFileName.c_str()))
    {
        IFC_RETURN(appModule.ProcessETL(processOrFileName));
        processingFinishedEvent.SetEvent();
    }
    else
    {
        ASSERT(false);
        IFC_RETURN(E_INVALIDARG);
    }

    processingFinishedEvent.wait();

    IFC_RETURN(appModule.Shutdown());
    return S_OK;
}

HRESULT StopImpl(
    _In_ const std::wstring& processName
    )
{
    // Make sure the event can be opened, this would get created inside of Run. If we can't open it here,
    // it means that tracing wasn't started
    wil::unique_event processingFinishedEvent;
    if (!processingFinishedEvent.try_open(c_drXamlEventName, EVENT_MODIFY_STATE))
    {
        wprintf(L"%s -start must be called before %s -stop.\n", processName.c_str(), processName.c_str());
        return E_INVALIDARG;
    }

    processingFinishedEvent.SetEvent();

    wprintf(L"Done, report can be found here: AppAnalysisReport.xml\n");
    return S_OK;
}

bool MatchesExtension(
    _In_z_ const wchar_t* file,
    _In_z_ const wchar_t* extensionToMatch
    )
{
    wchar_t actualExtension[_MAX_EXT] = { 0 };
    _wsplitpath_s(file, nullptr, 0, nullptr, 0, nullptr, 0, actualExtension, _countof(actualExtension));

    // do a case insensitive comparison
    return (_wcsicmp(actualExtension, extensionToMatch) == 0);
}

bool IsValidExecutableName(
    _In_z_ const wchar_t* string
    )
{
    return MatchesExtension(string, L".exe");
}

bool IsValidEtlName(
    _In_z_ const wchar_t* string
    )
{
    return MatchesExtension(string, L".etl");
}

bool IsValidProcessId(
    _In_z_ const wchar_t* string
    )
{
    // parse the string, make sure every character is a digit. if one isn't, return false
    size_t i =0;
    while (string[i] != '\0')
    {
        if (!iswdigit(string[i]))
        {
            return false;
        }
    }

    // if we successfully parsed the whole string, still make sure it's valid by the length.
    // For example: 1200199220202029 is not a valid process id beacuse no Windows process can
    // have this id.
    return (i <= MAX_PID_STRING_LENGTH);
}
