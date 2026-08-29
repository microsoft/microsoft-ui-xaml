// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <string>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
//
// The TraceSession class handles ETW Session creation and helps the standalone
// App Analysis application read etl files (or live events) in order to dispatch
// them to the App Analysis Eventprocessor.
//
class TraceSession final
{

public:
    TraceSession();
    virtual ~TraceSession();

    static HRESULT
        CreateInstance(
            _Out_ std::unique_ptr<TraceSession>* ppInstance
        );

    static DWORD MergeEtl(
        _In_ const std::wstring& userModeEtlName,
        _In_ const std::wstring& kernelEtlName,
        _In_ const std::wstring& mergedEtlName);

    static DWORD MergeEtl(
        _In_ const std::wstring& userModeEtlName,
        _In_ const std::wstring& mergedEtlName);

    HRESULT StartLiveTraceSession(
        _In_ const std::wstring& sessionName,
        _In_ const std::wstring& processName,
        _In_ const GUID& sessionGuid,
        _In_ ULONG enableFlags = 0
        );

    HRESULT StartLiveTraceSession(
        _In_ const std::wstring& sessionName,
        _In_ ULONG processId,
        _In_ const GUID& sessionGuid,
        _In_ ULONG enableFlags = 0
        );

    HRESULT StartEtlTraceSession(
        _In_ const std::wstring& fileName
        );

    HRESULT EnableProvider(
        _In_ const GUID& providerId
        );

    HRESULT ProcessTrace();

    HRESULT Shutdown();

    HRESULT Initialize();

    HRESULT GetTraceDetails(
        _Out_ std::wstring* logFile,
        _Out_opt_ UINT* eventsLost
        );

    void OnEventRecordCallback(
            _In_ PEVENT_RECORD pEventRecord
        );

private:

    HRESULT OpenTraceSession(
        _In_ const std::wstring& etlPathOrLoggerName);

    HRESULT StartTraceSession(
        _In_ ULONG enableFlags = 0
    );

    // Returns the status code from ::StartTrace, or ERROR_OUTOFMEMORY
    // if we couldn't create the session properties
    DWORD StartTraceSession(
        _In_ const std::wstring& sessionName,
        _In_ const std::wstring& etlFileName,
        _In_ const GUID& sessionGuid,
        _In_ ULONG enableFlags = 0
    );

    static DWORD MergeTracesInternal(
        _In_reads_z_(numTraceFiles) PCWSTR traceFileNames[],
        _In_ UINT numTraceFiles,
        _In_z_ PCWSTR mergedFileName
    );

    static EVENT_TRACE_PROPERTIES* CreateSessionProperties(
        _In_ const std::wstring& sessionName,
        _In_ const std::wstring& etlFileName,
        _In_ const GUID& sessionGuid,
        _In_ ULONG enableFlags = 0);

    EVENT_TRACE_LOGFILE CreateLogFile(
        _In_ const std::wstring& pathToEtlOrLoggerName);

    TRACEHANDLE m_hTrace;
    TRACEHANDLE m_hSession;
    GUID m_sessionGuid;
    std::wstring m_sessionName;
    std::unique_ptr<EVENT_TRACE_PROPERTIES[]> m_sessionProperties;
    std::vector<GUID> m_enabledProviders;
    std::wstring m_processName;
    std::wstring m_etlFileName;

    ULONG m_processId;

    bool m_liveTraceStarted;
    bool m_providersEnabled;

    typedef HRESULT(*pfnProcessEvent)(PEVENT_RECORD);
    pfnProcessEvent ProcessEvent{};

    volatile UINT m_fShutdown;

    static const unsigned long c_defaultMaxFileSize = 200;
};