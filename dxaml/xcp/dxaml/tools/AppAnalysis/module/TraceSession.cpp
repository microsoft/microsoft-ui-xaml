// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include "Module.h"
#include "TraceSession.h"
#include <unordered_map>
#include <wil\resource.h>
#include <ppltasks.h>
#include <cwctype>

using namespace Microsoft::Diagnostics::AppAnalysis;

//////////////////////////////////////////////////////////////////////////////////
////
HRESULT
TraceSession::CreateInstance(
    _Out_ std::unique_ptr<TraceSession>* instance
)
{
    std::unique_ptr<TraceSession> traceSession(new TraceSession());

    IFC_RETURN(traceSession->Initialize());
    *instance = std::move(traceSession);

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
VOID WINAPI
EventRecordCallback(
    _In_ PEVENT_RECORD pEventRecord
    )
{
    TraceSession* pThis = (TraceSession*)pEventRecord->UserContext;
    pThis->OnEventRecordCallback(pEventRecord);
}

////////////////////////////////////////////////////////////////////////////////
//
TraceSession::TraceSession()
    : m_hTrace(INVALID_PROCESSTRACE_HANDLE)
    , m_hSession(INVALID_PROCESSTRACE_HANDLE)
    , m_sessionGuid(GUID_NULL)
    , m_processId(0)
    , m_fShutdown(FALSE)
    , m_liveTraceStarted(false)
    , m_providersEnabled(false)
{
}

////////////////////////////////////////////////////////////////////////////////
//
TraceSession::~TraceSession()
{
    Shutdown();
    ::DeleteFile(m_etlFileName.c_str());
}

HRESULT
TraceSession::Initialize(
    )
{
    HMODULE dll = GetModuleHandle(L"Microsoft.Diagnostics.AppAnalysis.dll");
    if (!dll)
    {
        IFCSTATUS_RETURN(GetLastError());
    }

    ProcessEvent = (pfnProcessEvent)(GetProcAddress(dll, "ProcessEvent"));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::Shutdown()
{
    if (!InterlockedCompareExchange(&m_fShutdown, TRUE, FALSE))
    {
        if (m_providersEnabled)
        {
            // Disable the providers
            for (auto& guid : m_enabledProviders)
            {
                ::EnableTraceEx2(m_hSession, &guid, EVENT_CONTROL_CODE_DISABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, nullptr);
            }
        }

        if (m_liveTraceStarted)
        {
            // Stop the trace
            ::ControlTrace(m_hSession, m_sessionName.c_str(), m_sessionProperties.get(), EVENT_TRACE_CONTROL_STOP);
        }

        m_liveTraceStarted = false;

        TRACEHANDLE hTrace = m_hTrace;
        m_hTrace = INVALID_PROCESSTRACE_HANDLE;
        if (hTrace != INVALID_PROCESSTRACE_HANDLE)
        {
            ::CloseTrace(hTrace);
        }
    }

   return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::ProcessTrace()
{
    HRESULT hr = S_OK;
    
    TRACEHANDLE hTrace = m_hTrace;

    if (hTrace != INVALID_PROCESSTRACE_HANDLE)
    {
        ULONG status = ::ProcessTrace(&hTrace, 1, nullptr, nullptr);

        if (status != ERROR_SUCCESS &&
            status != ERROR_CANCELLED)
        {
            IFC(HRESULT_FROM_WIN32(status));
        }
    }
    else
    {
        IFC(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
    }

Cleanup:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::StartLiveTraceSession(
    _In_ const std::wstring& sessionName,
    _In_ const std::wstring& processName,
    _In_ const GUID& sessionGuid,
    _In_ ULONG enableFlags
    )
{
    m_sessionGuid = sessionGuid;
    m_sessionName = sessionName;
    m_processName = processName;

    IFC_RETURN(StartTraceSession(enableFlags));
    IFC_RETURN(OpenTraceSession(m_sessionName));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::StartLiveTraceSession(
    _In_ const std::wstring& sessionName,
    _In_ ULONG processId,
    _In_ const GUID& sessionGuid,
    _In_ ULONG enableFlags
    )
{
    m_sessionGuid = sessionGuid;
    m_sessionName = sessionName;
    m_processId = processId;

    IFC_RETURN(StartTraceSession(enableFlags));
    IFC_RETURN(OpenTraceSession(m_sessionName));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::StartEtlTraceSession(
    _In_ const std::wstring& pathToEtl
    )
{
    IFC_RETURN(OpenTraceSession(pathToEtl));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::EnableProvider(
    _In_ const GUID& provider)
{
    ENABLE_TRACE_PARAMETERS params = { 0 };
    params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.SourceId = m_sessionGuid;

    EVENT_FILTER_DESCRIPTOR filter = { 0 };

    if (!m_processName.empty())
    {
        filter.Ptr = reinterpret_cast<ULONG_PTR>(m_processName.c_str());
        filter.Size = static_cast<ULONG>((m_processName.length() + 1) *  sizeof(WCHAR));
        filter.Type = EVENT_FILTER_TYPE_EXECUTABLE_NAME;
    }
    else
    {
        ASSERT(m_processId != 0);
        filter.Ptr = reinterpret_cast<ULONG_PTR>(&m_processId);
        filter.Size = sizeof(DWORD);
        filter.Type = EVENT_FILTER_TYPE_PID;
    }

    params.EnableFilterDesc = &filter;
    params.FilterDescCount = 1;

    ULONG status = ::EnableTraceEx2(m_hSession, &provider, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, &params);
    if (status != ERROR_SUCCESS)
    {
        IFC_RETURN(HRESULT_FROM_WIN32(status));
    }

    IFCSTL_RETURN(m_enabledProviders.push_back(provider));
    m_providersEnabled = true;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::StartTraceSession(
    _In_ ULONG enableFlags)
{
    ASSERT(!IsEqualGUID(m_sessionGuid, GUID_NULL));
    ASSERT(!m_sessionName.empty());

    m_etlFileName = std::wstring(L"Unmerged_").append(m_sessionName);
    
    // the kernel provider session name is "NT Kernel Logger". No one likes spaces
    // in file names...
    if (enableFlags > 0)
    {
        m_etlFileName.erase(std::remove_if(m_etlFileName.begin(), m_etlFileName.end(), std::iswspace));
    }

    m_etlFileName.append(L".etl");

    DWORD status = StartTraceSession(m_sessionName, m_etlFileName, m_sessionGuid, enableFlags);

    // We dont' want to do this forever, 3 is a nice arbitrary number...
    int attemptsLeft = 3;

    while (status != ERROR_SUCCESS && attemptsLeft > 0)
    {
        // if no space on the disk, then don't write a file and try to start the session
        if (status == ERROR_DISK_FULL)
        {
            m_etlFileName.clear();
            status = StartTraceSession(m_sessionName, m_etlFileName, m_sessionGuid, enableFlags);
        }

        // If the trace session with these properties already exists, then stop the trace and restart it
        if (status == ERROR_ALREADY_EXISTS)
        {
            status = ::ControlTrace(m_hSession, m_sessionName.c_str(), m_sessionProperties.get(), EVENT_TRACE_CONTROL_STOP);
            // Since we don't need the session's property info, ERROR_MORE_DATA is ok and means that ETW has already stopped
            // the session
            if (status == ERROR_SUCCESS || status == ERROR_MORE_DATA)
            {
                status = StartTraceSession(m_sessionName, m_etlFileName, m_sessionGuid, enableFlags);
            }
        }
        --attemptsLeft;
    }

    IFCSTATUS_RETURN(status);

    m_liveTraceStarted = true;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
void
TraceSession::OnEventRecordCallback(
    _In_ PEVENT_RECORD pEventRecord
    )
{
    if (m_fShutdown == FALSE)
    {
        ProcessEvent(pEventRecord);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
EVENT_TRACE_LOGFILE
TraceSession::CreateLogFile(
    _In_ const std::wstring& pathToEtlOrLoggerName
    )
{
    EVENT_TRACE_LOGFILE traceLog = { 0 };

    // Enable Vista+ event callback style.
    traceLog.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    traceLog.Context = this;
    traceLog.EventRecordCallback = &EventRecordCallback;

    if (m_liveTraceStarted)
    {
        traceLog.LoggerName = const_cast<LPWSTR>(pathToEtlOrLoggerName.c_str());
        traceLog.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;
        // if this is the kernel logger, then set the flag
        if (pathToEtlOrLoggerName.compare(KERNEL_LOGGER_NAME) == 0)
        {
            traceLog.IsKernelTrace = TRUE;
        }
    }
    else
    {
        traceLog.LogFileName = const_cast<LPWSTR>(pathToEtlOrLoggerName.c_str());
    }

    return traceLog;
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT
TraceSession::OpenTraceSession(
    _In_ const std::wstring& pathToEtlOrLoggerName
    )
{
    EVENT_TRACE_LOGFILE etlLogFile = CreateLogFile(pathToEtlOrLoggerName);
    m_hTrace= ::OpenTrace(&etlLogFile);
    if (m_hTrace == INVALID_PROCESSTRACE_HANDLE)
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
EVENT_TRACE_PROPERTIES*
TraceSession::CreateSessionProperties(
    _In_ const std::wstring& sessionName,
    _In_ const std::wstring& etlFileName,
    _In_ const GUID& sessionGuid,
    _In_ ULONG enableFlags)
{
    // No log file being created so do not need to append size of that
    size_t bufferSize = 0;
    if (!etlFileName.empty())
    {
        bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.length() + 1) * sizeof(wchar_t) + (etlFileName.length() + 1) * sizeof(wchar_t);
    }
    else
    {
        bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.length() + 1)*sizeof(wchar_t);
    }

    std::unique_ptr<EVENT_TRACE_PROPERTIES[]> sessionProperties( new EVENT_TRACE_PROPERTIES[bufferSize]);

    // if out of memory, return nullptr
    if (!sessionProperties)
    {
        return nullptr;
    }

    ZeroMemory(sessionProperties.get(), bufferSize);

    sessionProperties.get()->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    sessionProperties.get()->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    sessionProperties.get()->Wnode.ClientContext = 1; //QPC clock resolution
    sessionProperties.get()->Wnode.Guid = sessionGuid;
    sessionProperties.get()->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    sessionProperties.get()->FlushTimer = 1;
    sessionProperties.get()->BufferSize = 32;
    sessionProperties.get()->MinimumBuffers = 48;
    sessionProperties.get()->MaximumBuffers = 48;

    if (enableFlags > 0)
    {
        sessionProperties.get()->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;
        sessionProperties.get()->EnableFlags =
            enableFlags;
    }

    if (!etlFileName.empty())
    {
        sessionProperties.get()->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;
        sessionProperties.get()->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        sessionProperties.get()->LogFileNameOffset = static_cast<ULONG>(sizeof(EVENT_TRACE_PROPERTIES) + (sessionName.length() + 1) * sizeof(wchar_t));
        sessionProperties.get()->MaximumFileSize = c_defaultMaxFileSize;

        // Copy the trace name to LoggerNameOffset location
        BYTE* pChar = reinterpret_cast<BYTE *>(sessionProperties.get());
        pChar += sessionProperties.get()->LoggerNameOffset;
        if (FAILED(::StringCchCopy(reinterpret_cast<wchar_t *>(pChar), (sessionName.length() + 1), sessionName.c_str())))
        {
            return nullptr;
        }

        // Copy the log file name to LogFileNameOffset location
        pChar = reinterpret_cast<BYTE *>(sessionProperties.get());
        pChar += sessionProperties.get()->LogFileNameOffset;
        if (FAILED(::StringCchCopy(reinterpret_cast<wchar_t *>(pChar), (etlFileName.length() + 1), etlFileName.c_str())))
        {
            return nullptr;
        }
    }

    return sessionProperties.release();
}

////////////////////////////////////////////////////////////////////////////////
//
HRESULT TraceSession::GetTraceDetails(
    _Out_ std::wstring* fileLogName,
    _Out_opt_ UINT* eventsLost
    )
{
    // This api should only be called when we've started a live trace session
    if (!m_liveTraceStarted)
    {
        return E_NOT_VALID_STATE;
    }

    // We only need to query the session if the caller cares about how many events were lost,
    // othwerwise we can just give them the etl log.
    if (eventsLost)
    {
        const size_t c_maxNameLength = 1024;
        const ULONG bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + 2 * c_maxNameLength;
        std::unique_ptr<EVENT_TRACE_PROPERTIES[]> sessionProperties(new EVENT_TRACE_PROPERTIES[bufferSize]);
        ZeroMemory(sessionProperties.get(), bufferSize);
        sessionProperties[0].Wnode.BufferSize = bufferSize;
        sessionProperties[0].Wnode.Guid = m_sessionProperties.get()->Wnode.Guid;
        sessionProperties[0].LoggerNameOffset = m_sessionProperties.get()->LoggerNameOffset;
        sessionProperties[0].LogFileNameOffset = m_sessionProperties.get()->LogFileNameOffset;
        sessionProperties[0].LogFileMode = m_sessionProperties.get()->LogFileMode;

        IFCSTATUS_RETURN(ControlTrace(m_hSession, m_sessionName.c_str(), sessionProperties.get(), EVENT_TRACE_CONTROL_QUERY));

        *eventsLost = sessionProperties[0].EventsLost;
    }

    *fileLogName = m_etlFileName;

    return S_OK;
}

DWORD
TraceSession::StartTraceSession(
    _In_ const std::wstring& sessionName,
    _In_ const std::wstring& etlFileName,
    _In_ const GUID& sessionGuid,
    _In_ ULONG enableFlags)
{
    m_sessionProperties.reset(CreateSessionProperties(sessionName, etlFileName, sessionGuid, enableFlags));
    if (!m_sessionProperties)
    {
        return ERROR_OUTOFMEMORY;
    }

    return ::StartTrace(static_cast<PTRACEHANDLE>(&m_hSession), sessionName.c_str(), m_sessionProperties.get());
}

DWORD
TraceSession::MergeEtl(
    _In_ const std::wstring& userModeEtlName,
    _In_ const std::wstring& kernelEtlName,
    _In_ const std::wstring& mergedEtlName)
{

    LPCWSTR traceFileNames[2] = { 0 };
    traceFileNames[0] = userModeEtlName.c_str();
    traceFileNames[1] = kernelEtlName.c_str();
    return MergeTracesInternal(traceFileNames, _countof(traceFileNames), mergedEtlName.c_str());
}

DWORD
TraceSession::MergeEtl(
    _In_ const std::wstring& userModeEtlName,
    _In_ const std::wstring& mergedEtlName)
{
    LPCWSTR traceFileName = userModeEtlName.c_str();
    return MergeTracesInternal(&traceFileName, 1, mergedEtlName.c_str());
}

// As defined in KernelTraceControl.h according to
// https://docs.microsoft.com/en-us/windows-hardware/test/wpt/custom-injection-of-system-information.

#define EVENT_TRACE_MERGE_EXTENDED_DATA_DEFAULT 0x000FFFFF

DWORD
TraceSession::MergeTracesInternal(
    _In_reads_z_(numTraceFiles) PCWSTR traceFileNames[],
    _In_ UINT numTraceFiles,
    _In_z_ PCWSTR mergedFileName
)
{
    // Export from KernelTraceControl.dll
    //ULONG
    //WINAPI
    //CreateMergedTraceFile(
    //    _In_  LPCWSTR wszMergedFileName,
    //    _In_reads_(cTraceFileNames) LPCWSTR wszTraceFileNames[],
    //    _In_  ULONG cTraceFileNames,
    //    _In_  DWORD dwExtendedDataFlags
    //)

    typedef ULONG(*pfnMergeEtlFiles)(LPCWSTR, LPCWSTR[], ULONG, DWORD);
    wil::unique_hmodule etlMerger(LoadLibrary(L"KernelTraceControl.dll"));

    if (!etlMerger)
    {
        return ERROR_MOD_NOT_FOUND;
    }

    pfnMergeEtlFiles mergeEtlFiles = (pfnMergeEtlFiles)GetProcAddress(etlMerger.get(), "CreateMergedTraceFile");
    return mergeEtlFiles(mergedFileName, traceFileNames, numTraceFiles, EVENT_TRACE_MERGE_EXTENDED_DATA_DEFAULT);
}
