// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ErrorContext.h>
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include <ErrorHandlerSettings.h>
#include "XamlTraceLogging.h"
#include <utility>
#include <MUX-ETWEvents.h>
#include <GraphicsUtility.h>
#include <DesignMode.h>
#include <ErrorContextStructure.h>
#include <IXamlTestHooks-errors.h>
#include <LayoutCycleDebugSettings.h>

#ifdef DBG
#include <Strsafe.h> // StringCchPrintf
#include <psapi.h> // EnumProcessModules, GetModuleInformation.

#include <dbghelp.h> // ImageNtHeader
#endif


// Define handle to the our TraceLoggingProvider
// provider guid -- 2dc72f6e-e4d1-5f58-3245-09a4243799dd
TRACELOGGING_DEFINE_PROVIDER(g_hTraceProvider,
    "Microsoft.UI.Xaml",
    (0x2dc72f6e, 0xe4d1, 0x5f58, 0x32, 0x45, 0x09, 0xa4, 0x24, 0x37, 0x99, 0xdd),
    TraceLoggingOptionMicrosoftTelemetry());

// These UIA error codes are ignored. Including the UIA header here would be
// ideal but it's kind of a huge tangly mess so for now we'll place these here.
// They arne't changing anytime soon.
#define UIA_E_ELEMENTNOTENABLED      0x80040200
#define UIA_E_ELEMENTNOTAVAILABLE    0x80040201

// Flag values used by the ErrorContext flags member.
const DWORD ERROR_CONTEXT_FLAG_HANDLED = 0x01;
const DWORD ERROR_CONTEXT_FLAG_FATAL   = 0x02;

// The maximum number of error contexts to store for each thread.
const ULONG MAX_ERROR_CONTEXTS = 5;

// The maximum number of warning contexts to store for each thread.
const size_t MAX_WARNING_CONTEXTS = 50;

// Used to mark that TLS couldn't be initialized.
const DWORD TLS_UNINITIALIZED = -1;

// The TLS slot used for ErrorContext storage.
static DWORD g_dwErrorContextTlsIndex = TLS_UNINITIALIZED;

// The TLS slot used for WarningContext storage.
static DWORD g_dwWarningContextTlsIndex = TLS_UNINITIALIZED;

// Used to indicate whether this DLL owns the error context TLS index.
static bool g_ownsErrorContextTlsIndex = false;

// Used to indicate whether this DLL owns the warning context TLS index.
static bool g_ownsWarningContextTlsIndex = false;

// Used to skip storing the thread errorInfo on the error context if it matches the last one we saved.
static thread_local void* g_lastThreadErrorInfo = nullptr;

// Used to suspend doing a FailFast on all stowed exceptions (if enabled)
static thread_local bool g_suspendFailFastOnStowedException = false;

// Used to suspend doing a FailFast only on a specific stowed exceptions (if enabled)
static thread_local HRESULT g_suspendFailFastOnSpecificStowedException = S_OK;

// Indicates if the current process has called an API to enable FailFast on stowed exceptions
static bool g_failFastOnErrors = false;

// Unfortunately some APIs use errors as part of their normal contract.
// If we know 100% of the time we see an error, that it is never truly
// an error, but part of an API contract, we ignore it from error context
// logging.
bool IsHrIgnoredForErrorContext(HRESULT hr)
{
    // UIA uses these errors extensively. Instead of making them ignored
    // everywhere we propagate them, we ignore them centrally here.

    return UIA_E_ELEMENTNOTENABLED == hr || UIA_E_ELEMENTNOTAVAILABLE == hr;
}

// Zeros out an ErrorContext. To be used after allocating a new ErrorContext,
// or after reclaiming an existing ErrorContext to be reused.
void InitializeErrorContext(_In_ ErrorContext* pContext)
{
    memset(pContext, 0, sizeof(ErrorContext));
}

// Zeros out a WarningContext. To be used after allocating a new WarningContext,
// or after reclaiming an existing WarningContext to be reused.
void InitializeWarningContext(_In_ WarningContext* context)
{
    memset(context, 0, sizeof(*context));
}

// Allocates an ErrorContext.
// Doesn't call into any XAML code, but directly uses an OS allocation function.
// This eliminates the possibility of recursion and excludes the ErrorContext
// from our leak detection.
ErrorContext* AllocateErrorContext()
{
    ErrorContext* pContext = static_cast<ErrorContext*>(
        HeapAlloc(GetProcessHeap(), 0, sizeof(ErrorContext)));

    if (pContext)
    {
        InitializeErrorContext(pContext);
    }

    return pContext;
}

// Allocates a WarningContext.
// Doesn't call into any XAML code, but directly uses an OS allocation function.
// This eliminates the possibility of recursion and excludes the WarningContext
// from our leak detection.
WarningContext* AllocateWarningContext()
{
    WarningContext* context = static_cast<WarningContext*>(
        HeapAlloc(GetProcessHeap(), 0, sizeof(WarningContext)));

    if (context)
    {
        InitializeWarningContext(context);
    }

    return context;
}

// Deallocates an ErrorContext that was allocated by AllocateErrorContext().
BOOL DeallocateErrorContext(ErrorContext* pContext)
{
    return HeapFree(GetProcessHeap(), 0, pContext);
}

// Deallocates a WarningContext that was allocated by AllocateWarningContext().
BOOL DeallocateWarningContext(WarningContext* context)
{
    return HeapFree(GetProcessHeap(), 0, context);
}

// Returns TRUE if the ErrorContext is marked as handled.
bool IsErrorContextHandled(_In_ ErrorContext* pContext)
{
    return ERROR_CONTEXT_FLAG_HANDLED & pContext->flags;
}

// Marks the ErrorContext as handled.
void MarkErrorContextHandled(_In_ ErrorContext* pContext)
{
    pContext->flags |= ERROR_CONTEXT_FLAG_HANDLED;
}

// Marks the ErrorContext as fatal.
void MarkErrorContextFatal(_In_ ErrorContext* pContext)
{
    pContext->flags |= ERROR_CONTEXT_FLAG_FATAL;
}

#define STOWED_EXCEPTION_INFORMATION_V2_SIGNATURE 'SE02'
#define STOWED_EXCEPTION_FORM_BINARY    0x01
#define STORE_STOWED_EXCEPTION_INFORMATION_THREAD_ID(s, t) \
    (s)->ThreadId = (t) >> 2;
#define STOWED_EXCEPTION_NESTED_TYPE(t) ((((((ULONG)(t)) >> 24) & 0xFF) <<  0) | \
                                         (((((ULONG)(t)) >> 16) & 0xFF) <<  8) | \
                                         (((((ULONG)(t)) >>  8) & 0xFF) << 16) | \
                                         (((((ULONG)(t)) >>  0) & 0xFF) << 24))

// Fills in the STOWED_EXCEPTION_INFORMATION_V2 member of the ErrorContext.
// To be used in preparation for a fail-fast.
void FillInStowedException(_In_ ErrorContext* pContext)
{
    // determine the HR to report
    HRESULT hr = 0;
    USHORT iFrame = 0;
    while (0 == hr && iFrame <= pContext->currentFrame && iFrame < pContext->frameCount)
    {
        hr = pContext->frameHRs[iFrame++];
    }
    if (0 == hr)
    {
        hr = E_FAIL;
    }

    PSTOWED_EXCEPTION_INFORMATION_V2 pStowedException = &pContext->stowedException;

    memset(pStowedException, 0, sizeof(*pStowedException));
    pStowedException->Header.Size = sizeof(*pStowedException);
    pStowedException->Header.Signature = STOWED_EXCEPTION_INFORMATION_V2_SIGNATURE;
    pStowedException->ResultCode = hr;
    pStowedException->ExceptionForm = STOWED_EXCEPTION_FORM_BINARY;
    STORE_STOWED_EXCEPTION_INFORMATION_THREAD_ID(pStowedException, pContext->threadId);
    pStowedException->StackTraceWordSize = sizeof(PVOID);
    pStowedException->StackTraceWords = pContext->frameCount;
    pStowedException->StackTrace = pContext->frameAddresses;

    pStowedException->NestedExceptionType = STOWED_EXCEPTION_NESTED_TYPE('XAML');
    pStowedException->NestedException = pContext;
}

// Fills in the STOWED_EXCEPTION_INFORMATION_V2 member of the WarningContext.
// To be used in preparation for a fail-fast.
void FillInStowedException(_In_ WarningContext* context)
{
    PSTOWED_EXCEPTION_INFORMATION_V2 stowedException = &context->stowedException;

    memset(stowedException, 0, sizeof(*stowedException));
    stowedException->Header.Size = sizeof(*stowedException);
    stowedException->Header.Signature = STOWED_EXCEPTION_INFORMATION_V2_SIGNATURE;
    stowedException->ResultCode = S_FALSE;
    stowedException->ExceptionForm = STOWED_EXCEPTION_FORM_BINARY;
    STORE_STOWED_EXCEPTION_INFORMATION_THREAD_ID(stowedException, context->threadId);
    stowedException->StackTraceWordSize = sizeof(PVOID);
    stowedException->StackTraceWords = context->frameCount;
    stowedException->StackTrace = context->frameAddresses;
    // STOWED_EXCEPTION_NESTED_TYPE uses 4-character constants. Since 'XAML' is already used above for ErrorContext,
    // 'XAMW' is picked for XAML WarningContext.
    stowedException->NestedExceptionType = STOWED_EXCEPTION_NESTED_TYPE('XAMW');
    stowedException->NestedException = context;
}

// Returns TRUE if the ErrorContext TLS slot has been initialized.
bool IsErrorContextTLSInitialized()
{
    return TLS_UNINITIALIZED != g_dwErrorContextTlsIndex && TLS_OUT_OF_INDEXES != g_dwErrorContextTlsIndex;
}

// Returns TRUE if the WarningContext TLS slot has been initialized.
bool IsWarningContextTLSInitialized()
{
    return TLS_UNINITIALIZED != g_dwWarningContextTlsIndex && TLS_OUT_OF_INDEXES != g_dwWarningContextTlsIndex;
}

// Gets the head of the linked list of ErrorContexts for this thread
// (may be null).
ErrorContext* GetErrorContextLinkedListHead()
{
    if (!IsErrorContextTLSInitialized())
    {
        return nullptr;
    }

    return static_cast<ErrorContext*>(TlsGetValue(g_dwErrorContextTlsIndex));
}

// Gets the head of the linked list of WarningContexts for this thread
// (may be null).
WarningContext* GetWarningContextLinkedListHead()
{
    if (!IsWarningContextTLSInitialized())
    {
        return nullptr;
    }

    return static_cast<WarningContext*>(TlsGetValue(g_dwWarningContextTlsIndex));
}

// Sets the head of the linked list of ErrorContexts for this thread.
bool SetErrorContextLinkedListHead(_In_opt_ ErrorContext* pContext)
{
    if (!IsErrorContextTLSInitialized())
    {
        return false;
    }

    return LOG_IF_WIN32_BOOL_FALSE(TlsSetValue(g_dwErrorContextTlsIndex, pContext));
}

// Sets the head of the linked list of WarningContexts for this thread.
bool SetWarningContextLinkedListHead(_In_opt_ WarningContext* context)
{
    if (!IsWarningContextTLSInitialized())
    {
        return false;
    }

    return LOG_IF_WIN32_BOOL_FALSE(TlsSetValue(g_dwWarningContextTlsIndex, context));
}

// Appends the provided extraInfo strings to the ContextBase instance. This optional
// string array can provide additional context-specific information in memory dumps.
void AppendContextBaseExtraInfo(_In_ ContextBase* context, _In_opt_ std::vector<std::wstring>* extraInfo)
{
    if (extraInfo)
    {
        for (const auto& extraInfoEntry : *extraInfo)
        {
            context->extraInfo.push_back(extraInfoEntry);
        }
    }
}

// Clears the extraInfo string array and threadErrorInfo of the ContextBase.
void ResetContextBaseExtraInfo(_In_ ContextBase* context)
{
    context->extraInfo.clear();
    context->extraInfo.shrink_to_fit(); // Get rid of any capacity to avoid mem leak reports by DxamlCoreTestHooks::PostTestCheckForLeaks.
    context->threadErrorInfo.Reset();
}

// Called when the XAML framework DLL or extension DLLs are loaded.
// If errorContextTlsIndex is a valid index, then the TLS index is initialized to that value;
// otherwise, we allocate a new TLS index.
extern "C"
[[nodiscard]]
_Check_return_ HRESULT ErrorContextGlobalInit(DWORD errorContextTlsIndex)
{
    if (!IsErrorContextTLSInitialized())
    {
        if (errorContextTlsIndex == TLS_UNINITIALIZED)
        {
            g_dwErrorContextTlsIndex = TlsAlloc();
            g_ownsErrorContextTlsIndex = true;
        }
        else
        {
            g_dwErrorContextTlsIndex = errorContextTlsIndex;
        }
    }

    return IsErrorContextTLSInitialized() ? S_OK : E_FAIL;
}

// Called when the XAML framework DLL or extension DLLs are loaded.
// If warningContextTlsIndex is a valid index, then the TLS index is initialized to that value;
// otherwise, we allocate a new TLS index.
// TODO: Consider simplifying by not using about TLS APIs or storing the index, and by using C++'s thread_local storage specifier:
// https://docs.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls?view=msvc-170
extern "C"
[[nodiscard]]
_Check_return_ HRESULT WarningContextGlobalInit(DWORD warningContextTlsIndex)
{
    if (!IsWarningContextTLSInitialized())
    {
        if (warningContextTlsIndex == TLS_UNINITIALIZED)
        {
            g_dwWarningContextTlsIndex = TlsAlloc();
            g_ownsWarningContextTlsIndex = true;
        }
        else
        {
            g_dwWarningContextTlsIndex = warningContextTlsIndex;
        }
    }

    return IsWarningContextTLSInitialized() ? S_OK : E_FAIL;
}

// Called when the XAML framework DLL or extension DLLs are unloaded.
// This de-allocates global resources used by the ErrorContext system if this DLL has initialized them,
// or just sets our TLS index to be uninitialized if it hasn't.
extern "C"
_Check_return_ HRESULT ErrorContextGlobalDeinit()
{
    if (IsErrorContextTLSInitialized())
    {
        if (g_ownsErrorContextTlsIndex)
        {
            TlsFree(g_dwErrorContextTlsIndex);
        }

        g_dwErrorContextTlsIndex = TLS_UNINITIALIZED;
    }

    return S_OK;
}

// Called when the XAML framework DLL or extension DLLs are unloaded.
// This de-allocates global resources used by the WarningContext system if this DLL has initialized them,
// or just sets our TLS index to be uninitialized if it hasn't.
extern "C"
_Check_return_ HRESULT WarningContextGlobalDeinit()
{
    if (IsWarningContextTLSInitialized())
    {
        if (g_ownsWarningContextTlsIndex)
        {
            TlsFree(g_dwWarningContextTlsIndex);
        }

        g_dwWarningContextTlsIndex = TLS_UNINITIALIZED;
    }

    return S_OK;
}

#ifdef DBG

void WriteLoadedModulesInfoToLogFile(HANDLE hLogFile)
{
    WCHAR wszBuffer[MAX_PATH + 70 + 1]; // each line = 70 chars, plus a filename, plus a null terminator
    HANDLE hCurrentProcess = GetCurrentProcess();
    HMODULE modules[1024];
    DWORD cbNeeded;
    size_t cModules;
    WCHAR wszModuleFileName[MAX_PATH + 1];
    MODULEINFO moduleInfo;

    // write the header
    if (FAILED(StringCchPrintf(
        wszBuffer, ARRAY_SIZE(wszBuffer),
        L"\r\nLoadedModules version=1\r\n%16s %16s %16s %16s %s\r\n",
        L"Base",
        L"Size",
        L"TimeDateStamp",
        L"Checksum",
        L"ImagePath")))
    {
        return;
    }

    if (!WriteFile(
        hLogFile,
        wszBuffer,
        static_cast<DWORD>(wcslen(wszBuffer) * sizeof(WCHAR)),
        NULL,
        NULL))
    {
        return;
    }

    if (!EnumProcessModules(hCurrentProcess, modules, sizeof(modules), &cbNeeded))
    {
        return;
    }

    cModules = cbNeeded / sizeof(HMODULE);
    for (size_t i = 0; i < cModules; ++i)
    {
        if (GetModuleFileName(modules[i], wszModuleFileName, ARRAY_SIZE(wszModuleFileName)))
        {
            if (GetModuleInformation(hCurrentProcess, modules[i], &moduleInfo, sizeof(moduleInfo)))
            {
                PIMAGE_NT_HEADERS pImageHeaders = ImageNtHeader(modules[i]);


                if (pImageHeaders)
                {
                    if (SUCCEEDED(StringCchPrintf(
                        wszBuffer, ARRAY_SIZE(wszBuffer),
                        L"%16p %16X %16X %16X %s\r\n",
                        moduleInfo.lpBaseOfDll,
                        moduleInfo.SizeOfImage,
                        pImageHeaders->FileHeader.TimeDateStamp,
                        pImageHeaders->OptionalHeader.CheckSum,
                        wszModuleFileName)))
                    {
                        WriteFile(
                            hLogFile,
                            wszBuffer,
                            static_cast<DWORD>(wcslen(wszBuffer) * sizeof(WCHAR)),
                            NULL,
                            NULL);
                    }
                }
            }
        }
    }
}

HANDLE OpenErrorContextLogFile()
{
    WCHAR wszLogFileName[MAX_PATH + 1];
    HANDLE hLogFile = NULL;

    DWORD cchLogFileName = GetEnvironmentVariable(L"ErrorContextLogFile", wszLogFileName, ARRAY_SIZE(wszLogFileName));
    if (0 == cchLogFileName || cchLogFileName > ARRAY_SIZE(wszLogFileName))
    {
        return NULL;
    }

    hLogFile = CreateFile(
        wszLogFileName,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE == hLogFile)
    {
        return NULL;
    }

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hLogFile, 0, NULL, FILE_END))
    {
        CloseHandle(hLogFile);
        return NULL;
    }

    return hLogFile;
}

void WriteErrorContextToLogFile(HANDLE hLogFile, _In_ ErrorContext* pContext)
{
    WCHAR wszBuffer[256];

    if (!pContext->frameCount)
    {
        return;
    }

    // write the header
    if (FAILED(StringCchPrintf(
        wszBuffer, ARRAY_SIZE(wszBuffer),
        L"\r\nErrorContext version=1 originalHr=%X currentHr=%X currentFrame=%d\r\n",
        pContext->frameHRs[0],
        pContext->frameHRs[pContext->currentFrame],
        pContext->currentFrame)))
    {
        return;
    }

    if (!WriteFile(
        hLogFile,
        wszBuffer,
        static_cast<DWORD>(wcslen(wszBuffer) * sizeof(WCHAR)),
        NULL,
        NULL))
    {
        return;
    }

    // write out the stack
    for (USHORT i = 0; i < pContext->frameCount; ++i)
    {
        if (FAILED(StringCchPrintf(wszBuffer, ARRAY_SIZE(wszBuffer), L"%p\r\n", pContext->frameAddresses[i])))
        {
            return;
        }

        if (!WriteFile(
            hLogFile,
            wszBuffer,
            static_cast<DWORD>(wcslen(wszBuffer) * sizeof(WCHAR)),
            NULL,
            NULL))
        {
            return;
        }
    }
}

void CloseErrorContextLogFile(HANDLE hLogFile)
{
    CloseHandle(hLogFile);
}

#endif

// If properly configured, logs the accumulated errors on this thread out
// to a log file. This is called when a thread is deinitialized
// (graceful shutdown) and when we suspend.
//
// Logging will only occur:
//  - on DBG builds
//  - if the ErrorContextLogFile environment variable is set to the
//    location of the file to write to
//  - on suspend, if the ErrorContextLogOnSuspend environment variable is
//    set to true
//  - on shutdown, if the ErrorContextLogOnShutdown environvment variable
//    is set to true
_Check_return_ HRESULT ErrorContextLogAccumulatedErrors(ErrorContextLog::ErrorContextLogType logType)
{
    HRESULT hr = S_OK;

#ifdef DBG
    WCHAR envVarValue[5];
    HANDLE hLogFile = NULL;

    ErrorContext* pCurrent = GetErrorContextLinkedListHead();
    if (!pCurrent)
    {
        goto Cleanup;
    }

    if (ErrorContextLog::ErrorContextLogType::Suspend == logType)
    {
        DWORD cchEnvVarValue = GetEnvironmentVariable(L"ErrorContextLogOnSuspend", envVarValue, ARRAY_SIZE(envVarValue));
        if (0 == cchEnvVarValue || cchEnvVarValue > ARRAY_SIZE(envVarValue) || _wcsicmp(envVarValue, L"true") != 0)
        {
            goto Cleanup;
        }
    }
    else if (ErrorContextLog::ErrorContextLogType::Shutdown == logType)
    {
        DWORD cchEnvVarValue = GetEnvironmentVariable(L"ErrorContextLogOnShutdown", envVarValue, ARRAY_SIZE(envVarValue));
        if (0 == cchEnvVarValue || cchEnvVarValue > ARRAY_SIZE(envVarValue) || _wcsicmp(envVarValue, L"true") != 0)
        {
            goto Cleanup;
        }
    }
    else
    {
        goto Cleanup;
    }

    hLogFile = OpenErrorContextLogFile();
    if (!hLogFile)
    {
        goto Cleanup;
    }

    WriteLoadedModulesInfoToLogFile(hLogFile);

    while (pCurrent)
    {
        WriteErrorContextToLogFile(hLogFile, pCurrent);

        pCurrent = pCurrent->next;
    }

Cleanup:
    if (hLogFile)
    {
        CloseErrorContextLogFile(hLogFile);
    }
#endif

    RRETURN(hr);
}


// Called when a thread is deinitialized. This de-allocates memory the
// error context system has allocated that was specific to the thread.
extern "C"
_Check_return_ HRESULT ErrorContextThreadDeinit()
{
    if (!IsErrorContextTLSInitialized())
    {
        return S_OK;
    }

    IGNOREHR(ErrorContextLogAccumulatedErrors(ErrorContextLog::ErrorContextLogType::Shutdown));

    ErrorContext* pCurrent = GetErrorContextLinkedListHead();
    if (!pCurrent)
    {
        return S_OK;
    }

    SetErrorContextLinkedListHead(NULL);

    while (pCurrent)
    {
        ErrorContext* pNext = pCurrent->next;
        ResetContextBaseExtraInfo(pCurrent);
        DeallocateErrorContext(pCurrent);
        pCurrent = pNext;
    }

    return S_OK;
}

// Called when a thread is deinitialized. This de-allocates memory the
// error context system has allocated that was specific to the thread.
extern "C"
_Check_return_ HRESULT WarningContextThreadDeinit()
{
    if (!IsWarningContextTLSInitialized())
    {
        return S_OK;
    }

    WarningContext* current = GetWarningContextLinkedListHead();
    if (!current)
    {
        return S_OK;
    }

    SetWarningContextLinkedListHead(nullptr);

    while (current)
    {
        WarningContext* next = current->next;
        ResetContextBaseExtraInfo(current);
        DeallocateWarningContext(current);
        current = next;
    }

    return S_OK;
}

// Captures a new ErrorContext.
// This function should be called as closely as possible to the error origination point.
extern "C"
bool CaptureErrorContext(HRESULT failedFrameHR, INSTRUCTION_ADDRESS callerReturnAddress, _In_opt_ CONTEXT* contextRecord, _In_opt_ std::vector<std::wstring>* extraInfo)
{
    ErrorContext* pContext = NULL;
    ErrorContext* pHead = GetErrorContextLinkedListHead();

#ifndef DBG
    // We only log here on release builds.  Debug builds will call LOG_HR_MSG from the IFC macro(s) and that will include file/line/function information that is
    // not available here.  Release builds set RESULT_DIAGNOSTICS_LEVEL to 0 so the only thing that we are capable of logging is the HRESULT and the caller return
    // address.
    //
    // Slightly sketchy code alert - We use a method out of the wil::details namespace so that we can provide the caller return address.  The WIL macros will try
    // to capture that for themselves which will give this method which is not helpful.
    wil::details::ReportFailure_Hr<wil::FailureType::Log>(_ReturnAddress(), 0, nullptr, nullptr, nullptr, callerReturnAddress, failedFrameHR);
#endif

    // walk the linked list, looking for an error context to re-use
    if (pHead)
    {
        ErrorContext* pCurrent = pHead;
        unsigned int cErrorContextLinkedList = 1;
        while (pCurrent->next && cErrorContextLinkedList++ < MAX_ERROR_CONTEXTS - 1)
        {
            pCurrent = pCurrent->next;
        }
        if (pCurrent->next)
        {
            pContext = pCurrent->next;
            pCurrent->next = NULL;
            ResetContextBaseExtraInfo(pContext);
            InitializeErrorContext(pContext);
        }
    }

    if (!pContext)
    {
        // Couldn't find an ErrorContext to re-use, so allocate a new one.
        pContext = AllocateErrorContext();
    }

    if (!pContext)
    {
        // Allocation failed (OOM).
        return false;
    }

    // Don't skip any frames, as the optimizer can cause differences between chk and fre regarding which functions put return addresses
    // on the stack and which just fix up parameter registers (like rcx, rdx, r8, r9) and do a "jmp" without ever putting their own
    // return address on the stack.  Note that this can be the case even for "__declspec(noinline)" functions.  Instead, search for the
    // callerFrameAddress and pack down appropriately.  This does lose one or two frames from the max stack depth, but that depth is
    // large, so not a big deal.
    pContext->frameCount = CaptureStackBackTrace(
        0, // skip 0 frames
        ARRAY_SIZE(pContext->frameAddresses),
        pContext->frameAddresses,
        NULL);

    // Force pContext->frameCount to be at least 1 even if CaptureStackBackTrace() isn't happy and returns 0.
    // This avoids potential problems that would otherwise exist elsewhere in this function and elsewhere in this file.
    if (pContext->frameCount == 0)
    {
        pContext->frameAddresses[0] = _ReturnAddress();
        pContext->frameCount = 1;
    }

    USHORT findCallerFrameIndex;
    USHORT searchBound = pContext->frameCount;
    for (findCallerFrameIndex = 0; findCallerFrameIndex != searchBound; findCallerFrameIndex++)
    {
        if (callerReturnAddress == pContext->frameAddresses[findCallerFrameIndex])
        {
            break;
        }
    }

    // Do the pack-down, unless somehow the callerFrameAddress wasn't found, in which case leave the probably-broken crawl alone.
    if (findCallerFrameIndex != searchBound && findCallerFrameIndex > 1)
    {
        USHORT framesToSkip = findCallerFrameIndex - 1;

        USHORT packEndIndex = pContext->frameCount - framesToSkip;
        for (USHORT iFrame = 0; iFrame < packEndIndex; iFrame += 1)
        {
            pContext->frameAddresses[iFrame] = pContext->frameAddresses[iFrame + framesToSkip];
        }

        USHORT zeroEndIndex = pContext->frameCount;
        for (USHORT iFrame = packEndIndex; iFrame < zeroEndIndex; iFrame += 1)
        {
            pContext->frameAddresses[iFrame] = NULL;
        }

        pContext->frameCount -= framesToSkip;
    }

    if (contextRecord != nullptr)
    {
        pContext->contextRecord = *contextRecord;
    }
    else
    {
        memset(&pContext->contextRecord, 0, sizeof(pContext->contextRecord));
    }

    // Copy the optional extra info fields into the context instance.
    AppendContextBaseExtraInfo(pContext, extraInfo);

    // Save the error info on the thread, if any.
    void* currentThreadErrorInfo = nullptr;
    if (SUCCEEDED(::GetErrorInfo(0, &pContext->threadErrorInfo)))
    {
        currentThreadErrorInfo = (void*)pContext->threadErrorInfo.Get();
        // Put the error info back on the thread, since GetErrorInfo removes it.
        if (pContext->threadErrorInfo.Get())
        {
            IGNOREHR(::SetErrorInfo(0, pContext->threadErrorInfo.Get()));

            // Don't keep the error info if it is the same one we saved last time.
            // Note: This just does a raw pointer check which might miss if the same
            // memory gets reused, but this avoids g_lastThreadErrorInfo holding a reference.
            if ((void*)pContext->threadErrorInfo.Get() == g_lastThreadErrorInfo)
            {
                pContext->threadErrorInfo.Reset();
            }
        }
    }
    g_lastThreadErrorInfo = currentThreadErrorInfo;

    GetSystemTimeAsFileTime(&pContext->captureTime);
    pContext->threadId = GetCurrentThreadId();
    pContext->next = pHead;

    if (pHead)
    {
        MarkErrorContextHandled(pHead);
    }

    // The new TAEF dev test suite registers for this callback to perform
    // its own stack capture and dump creation.
    static auto errorHandlerSettings = ErrorHandling::GetErrorHandlingSettings();
    if (errorHandlerSettings)
    {
        wil::FailureInfo wilFailure = {};
        wilFailure.type = wil::FailureType::Return;
        wilFailure.hr = failedFrameHR;
        wilFailure.threadId = pContext->threadId;
        wilFailure.returnAddress = _ReturnAddress();
        wilFailure.callerReturnAddress = callerReturnAddress;

        ErrorHandling::XamlFailureInfo failure = {};
        failure.pFailureInfo = &wilFailure;
        failure.pErrorContext = pContext;
        errorHandlerSettings->ExecuteErrorHandlingCallbackIfSet(failure);
    }

    return SetErrorContextLinkedListHead(pContext);
}


// Called to update the existing ErrorContext with new information as the stack unwinds during error propagation.
// The caller supplies the current frame address and the current HR as of that frame.
// If the frame address is found within the error context, the context is updated with the HR, and TRUE is returned.
// If the frame address can't be found, false is returned.
extern "C"
bool UpdateErrorContext(HRESULT frameHR, INSTRUCTION_ADDRESS callerReturnAddress, _In_opt_ std::vector<std::wstring>* extraInfo)
{
    ErrorContext* pContext = GetErrorContextLinkedListHead();
    if (!pContext || IsErrorContextHandled(pContext))
    {
        return false;
    }

    bool fFoundNewFrame = false;

    // For debugging purposes only:
    pContext->currentMatchAddress = callerReturnAddress;

    for (USHORT iFrame = pContext->currentFrame; iFrame < (pContext->frameCount - 1) && !fFoundNewFrame; ++iFrame)
    {
        INSTRUCTION_ADDRESS address = pContext->frameAddresses[iFrame + 1];

        if (address == callerReturnAddress)
        {
            pContext->currentFrame = iFrame;

            // If the current frame has already matched once and is trying to match again, it means the
            // first match was an IFC() macro doing its thing, and the second match is a subsequent IFC()
            // macro being used from the same frame - this can legitimately happen in the case of inlining.
            // In that case the HRESULTs should match, but in case they don't, only keep the first one (the
            // first one should line up well with the stack crawl).  Don't go so far as to treat a
            // non-matching HRESULT in the same frame as a new failure, in case a syntactic calling frame
            // wants to convert to a different HRESULT.
            if (pContext->frameHRs[iFrame] == 0)
            {
                pContext->frameHRs[iFrame] = frameHR;
            }

            // Append the optional string array to the extraInfo that may already exist. 
            AppendContextBaseExtraInfo(pContext, extraInfo);

            fFoundNewFrame = true;

            // When recursion is used, the return address pattern can have repeats.  For example, something
            // like A, B, A, A, B, B, B, A, A.  Because we only consider return addresses and not the addresses
            // of the return addresses on the stack, this causes ambiguity re. which frame is really the current
            // frame failing an IFC().  Stop at the leaf-most match, to avoid accidentally skipping over too
            // many frames.  In this example, this avoids throwing out all the B's too soon when matching
            // on A.  In an example like this, the frameHRs array will be sparse, and some of the "middle"
            // HRESULTS may not be saved (which would only matter if they're different).  But at least the stack
            // crawl for the original failure, and the first HRESULT, will be saved.
            break;
        }
    }

    return fFoundNewFrame;
}


// Clears the existing error context.
extern "C"
void ClearErrorContext()
{
    ErrorContext* pContext = GetErrorContextLinkedListHead();
    if (pContext)
    {
        MarkErrorContextHandled(pContext);
    }
}

// Deletes the existing warning contexts with the provided type.
extern "C"
void ClearWarningContexts(WarningContextLog::WarningContextType type)
{
    WarningContext* head = GetWarningContextLinkedListHead();
    WarningContext* current = head;

    while (current)
    {
        if (current->type == type)
        {
            if (head == current)
            {
                head = current->next;
            }

            WarningContext* next = current->next;
            DeallocateWarningContext(current);
            current = next;
        }
        else
        {
            current = current->next;
        }
    }

    SetWarningContextLinkedListHead(head);
}

// Called by DxamlCoreTestHooks::PostTestCheckForLeaks to discard the extra info strings
// after a test completes and before ErrorContextThreadDeinit() is called.
extern "C"
void ResetErrorContextsExtraInfo()
{
    if (IsErrorContextTLSInitialized())
    {
        ErrorContext* current = GetErrorContextLinkedListHead();

        while (current)
        {
            ErrorContext* next = current->next;
            ResetContextBaseExtraInfo(current);
            current = next;
        }
    }
}


bool HasCapturedErrorContexts()
{
    return GetErrorContextLinkedListHead() != NULL;
}


DWORD GetErrorContextTlsIndex()
{
    return g_dwErrorContextTlsIndex;
}


DWORD GetWarningContextTlsIndex()
{
    return g_dwWarningContextTlsIndex;
}


__declspec(noinline) INSTRUCTION_ADDRESS GetCallerReturnAddressFromDirectCaller(INSTRUCTION_ADDRESS directCallerReturnAddress)
{
    // Default to directCallerReturnAddress, which would be wrong, but at least would let an error context get generated anyway.
    // We expect to find directCallerReturnAddress in the current stack backtrace, so this default will rarely if ever stand.
    INSTRUCTION_ADDRESS callerReturnAddress = directCallerReturnAddress;

    // A total of 5 return addresses are needed max.  The return addresses will point to an instruction within each of the
    // functions in this list, max:
    //   * this function
    //   * the On[New]FailureEncountered
    //   * the UniqueError::On[New]Failure (optional frame)
    //   * the "caller" of the IFC() macro
    //   * the caller of the "caller" of the IFC() macro (this is the one we actually want)
    // Some may be missing due to unpredictable inlining however (and whether UniqueError::On[New]Failure is used).

    // Finding the callerReturnAddress this way makes the module notably smaller at the cost of just a little speed when errors
    // are propagating up a stack that has a series of IFC() macros on the way up.

    INSTRUCTION_ADDRESS returnAddresses[5];
    USHORT count;
    count = CaptureStackBackTrace(
        0, // skip 0 frames, since inlining can be unpredictable.
        ARRAY_SIZE(returnAddresses),
        returnAddresses,
        NULL);
    if (count != 0)
    {
        for (USHORT iter = 0; iter < count - 1; iter++)
        {
            if (returnAddresses[iter] == directCallerReturnAddress)
            {
                callerReturnAddress = returnAddresses[iter + 1];
            }
        }
    }

    return callerReturnAddress;
}

// Processes a null terminated string list of comma delimitted process names (ex: "Calculator.exe,ShellHostExperience.exe").
// Returns true if the current process name is part of the list.
bool DoesStringContainCurrentProcess(wchar_t* inputString)
{
    int len = static_cast<int>(wcslen(inputString));

    // The string should be a comma-separated list of .exe names.
    wchar_t* current = inputString;
    wchar_t* const end = current + len;
    while (current < end)
    {
        wchar_t* currentEnd = wcsstr(current, L",");
        if (currentEnd == nullptr)
        {
            currentEnd = end;
        }
        else
        {
            *currentEnd = L'\0';
        }

        // If the name ends in ".exe" and a module of that name is loaded,
        // then consider this app as part of the opted-in/exclusion mechanism.
        const wchar_t dotExe[] = L".exe";
        const size_t dotExeLen = ARRAYSIZE(dotExe) - 1;
        if (static_cast<size_t>(currentEnd - current) > dotExeLen &&
            wcscmp(currentEnd - dotExeLen, dotExe) == 0 &&
            ::GetModuleHandle(current) != nullptr)
        {
            return true;
        }

        current = currentEnd + 1;
    }

    return false;
}

// With this method, for a process to opt-in to FailFast on stowed exceptions:
// (1) FailFastOnAnyFailure must be allowed, via the AllowFailFastOnAnyFailure XAML feature key.
// (2) The process must be opted in part of the HKLM\Software\Microsoft\WinUI\XAML\FailFastOnAnyFailureProcesses regkey
//     a. Specific (ex: "Calculator.exe,ShellHostExperience.exe")
//     b. Generic  ("*"). We will allow all processes to opt-in, except if being part of the exclusion list provided via:
//        HKLM\Software\Microsoft\WinUI\XAML\FailFastDisableOnSpecificProcesses regkey (ex: "LogonUI.exe")
bool IsFailFastOnAnyFailureEnabledViaRegKey()
{
    static bool s_isProcessOptIn = false;
    static bool s_isProcessOptInEvaluated = false;
    if (!s_isProcessOptInEvaluated)
    {
        bool isOptIn = false;
        wchar_t buffer[256] = {};
        DWORD size = sizeof(buffer);

        auto returnVal = RegGetValueW(
            HKEY_LOCAL_MACHINE,
            XAML_ROOT_KEY,
            L"FailFastOnAnyFailureProcesses",
            RRF_RT_REG_SZ,
            NULL,
            buffer,
            &size);

        // Do we want to fail fast on any or a particular process
        if (returnVal == ERROR_SUCCESS)
        {
            if ((buffer[0] == L'*') && (buffer[1] == L'\0'))
            {
                // We're being asked to enable failfast everywhere
                isOptIn = true;

                // But before we do that, let's look at our exclusion list
                buffer[0] = L'\0';
                size = sizeof(buffer);
                if (RegGetValueW(
                        HKEY_LOCAL_MACHINE,
                        XAML_ROOT_KEY,
                        L"FailFastDisableOnSpecificProcesses",
                        RRF_RT_REG_SZ,
                        NULL,
                        buffer,
                        &size) == ERROR_SUCCESS)
                {
                    isOptIn = !DoesStringContainCurrentProcess(buffer);
                }
            }
            else if (static_cast<int>(wcslen(buffer)) > 0)
            {
                isOptIn = DoesStringContainCurrentProcess(buffer);
            }
        }

        s_isProcessOptIn = isOptIn;
        s_isProcessOptInEvaluated = true;
    }

    if (!s_isProcessOptIn)
    {
        return false;
    }

    // The process is opt-in, so now check if FailFast is allowed.

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    bool isEnabled = runtimeEnabledFeatureDetector->IsFeatureEnabled(
            RuntimeFeatureBehavior::RuntimeEnabledFeature::AllowFailFastOnAnyFailure);

    return isEnabled;
}

// Calling this enables/disables FailFastOnErrors for the process.
void SetProcessFailFastOnErrors(bool enabled)
{
    g_failFastOnErrors = enabled;
}

bool GetProcessFailFastOnErrors()
{
    return g_failFastOnErrors;
}

// Returns whether this process is set to FailFast on any failure.
bool IsFailFastOnAnyFailureEnabled()
{
    // Note:  Using the FailFastOnErrors function (via DebugSettings.FailFastOnErrors or
    //        currently the private FrameworkApplicationStatics::EnableFailFastOnStowedException
    //        API) is respected only when not running in the designer.  The (older) regkey
    //        is always respected.
    return
        (g_failFastOnErrors && !DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
        || IsFailFastOnAnyFailureEnabledViaRegKey();
}

// Check against a list of HR's which we handle with special logic, and for which
// which we never want FailFastOnStoredException behavior.
bool AllowFailFastOnStowedExceptionForError(HRESULT failureHR)
{
    return !GraphicsUtility::IsDeviceLostError(failureHR);    // We recover from device lost errors.
}

// Returns true if this process should currently FailFast on the given failure
bool ShouldFailFastOnStowedException(HRESULT failedFrameHR)
{
    return
        (!g_suspendFailFastOnStowedException &&
         (g_suspendFailFastOnSpecificStowedException == S_OK || failedFrameHR != g_suspendFailFastOnSpecificStowedException) &&
         IsFailFastOnAnyFailureEnabled() &&
         AllowFailFastOnStowedExceptionForError(failedFrameHR));
}


// Helper function that combines several of the above operations.
// Intended to be called when originating a new failure.
//
//  1) captures a new error context: CaptureErrorContext()
//  2) updates the captured context: UpdateErrorContext()
//
// Returns TRUE if both operations succeed.
extern "C"
__declspec(noinline) bool OnNewFailureEncountered(HRESULT failedFrameHR, INSTRUCTION_ADDRESS directCallerReturnAddress, _In_opt_ CONTEXT* contextRecord)
{
    if (!IsErrorContextTLSInitialized())
    {
        // early out to save unnecessary work
        return false;
    }

    if (IsHrIgnoredForErrorContext(failedFrameHR))
    {
        return false;
    }

    if (directCallerReturnAddress == NULL)
    {
        // This means OnNewFailure<>() wasn't used.
        directCallerReturnAddress = _ReturnAddress();
    }

    INSTRUCTION_ADDRESS callerReturnAddress = GetCallerReturnAddressFromDirectCaller(directCallerReturnAddress);

    TraceFailureEncounteredInfo(reinterpret_cast<uint64_t>(callerReturnAddress), failedFrameHR);

    if (!CaptureErrorContext(failedFrameHR, callerReturnAddress, contextRecord))
    {
        return false;
    }

    if (!UpdateErrorContext(failedFrameHR, callerReturnAddress))
    {
        ClearErrorContext();
        return false;
    }

    if (ShouldFailFastOnStowedException(failedFrameHR))
    {
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(failedFrameHR);
    }

    return true;
}


// Helper function that combines several of the above operations.
// Intended to be called when either originating a new failure or propagating an existing failure.
//
//  1) propagation case:
//     - first, try to update the existing error context: UpdateErrorContext()
//     - if that succeeds, return TRUE
//
//  2) else, origination case:
//     - capture a new error context: CaptureErrorContext()
//     - update it: UpdateErrorContext()
//     - returns TRUE if both operations succeed
extern "C"
__declspec(noinline) bool OnFailureEncountered(HRESULT failedFrameHR, INSTRUCTION_ADDRESS directCallerReturnAddress, _In_opt_ CONTEXT* contextRecord, _In_opt_ std::vector<std::wstring>* extraInfo)
{
    if (!IsErrorContextTLSInitialized())
    {
        // early out to save unnecessary work
        return false;
    }

    if (IsHrIgnoredForErrorContext(failedFrameHR))
    {
        return false;
    }

    if (directCallerReturnAddress == NULL)
    {
        // This means OnFailure<>() wasn't used.
        directCallerReturnAddress = _ReturnAddress();
    }

    INSTRUCTION_ADDRESS callerReturnAddress = GetCallerReturnAddressFromDirectCaller(directCallerReturnAddress);

    if (UpdateErrorContext(failedFrameHR, callerReturnAddress, extraInfo))
    {
        return true;
    }

    if (!CaptureErrorContext(failedFrameHR, callerReturnAddress, contextRecord, extraInfo))
    {
        return true;
    }

    if (!UpdateErrorContext(failedFrameHR, callerReturnAddress))
    {
        ClearErrorContext();
        return false;
    }

    TraceFailureEncounteredInfo(reinterpret_cast<uint64_t>(callerReturnAddress), failedFrameHR);

    if (ShouldFailFastOnStowedException(failedFrameHR))
    {
        FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(failedFrameHR);
    }

    return true;
}


// Creates a WarningContext with the provided info and sets it as the head of the warning context linked list.
// Only MAX_WARNING_CONTEXTS instances are kept in the list. The last one is recycled in case that limit has already been reached.
extern "C"
__declspec(noinline) bool OnWarningEncountered(WarningContextLog::WarningContextType type, _In_opt_ std::vector<std::wstring>* extraInfo, size_t framesToSkip)
{
    if (!IsWarningContextTLSInitialized())
    {
        // Early out to save unneccessary work
        return false;
    }

    WarningContext* context = nullptr;
    WarningContext* head = GetWarningContextLinkedListHead();

    // Walk the linked list of WarningContexts looking for a context to re-use if the max of MAX_WARNING_CONTEXTS was already allocated.
    if (head)
    {
        WarningContext* current = head;
        size_t cWarningContextLinkedList = 1;

        // Increase the max warning count when the tracing level is High or above.
        const size_t max_warning_contexts = MAX_WARNING_CONTEXTS * (LayoutCycleDebugSettings::ShouldTrace(DirectUI::LayoutCycleTracingLevel::High) ? 3 : 1);
        while (current->next && ++cWarningContextLinkedList < max_warning_contexts)
        {
            current = current->next;
        }

        if (current->next)
        {
            context = current->next;
            current->next = nullptr;
            ResetContextBaseExtraInfo(context);
            InitializeWarningContext(context);
        }
    }

    if (!context)
    {
        context = AllocateWarningContext();
    }

    if (!context)
    {
        // Allocation failed (OOM).
        return false;
    }

    context->type = type;

    // Record the stack trace, skipping OnWarningEncountered and its top callers.
    context->frameCount = CaptureStackBackTrace(
        static_cast<ULONG>(framesToSkip),
        ARRAY_SIZE(context->frameAddresses),
        context->frameAddresses,
        nullptr);

    AppendContextBaseExtraInfo(context, extraInfo);

    GetSystemTimeAsFileTime(&context->captureTime);
    context->threadId = GetCurrentThreadId();
    context->next = head;

    // If this is a LayoutCycle warning and tracing is on, then trace the warning now.
    // Since the current implementation is that WarningContexts are only generated at
    // the set LayoutCycleTracingLevel, all we need to check here is that it isn't off.
    if (type == WarningContextLog::WarningContextType::LayoutCycle &&
        LayoutCycleDebugSettings::GetTracingLevel() != DirectUI::LayoutCycleTracingLevel::None)
    {
        std::wstring trace;
        trace.reserve(256);
        trace.append(L"[LayoutCycleTracing] ");

        if (extraInfo)
        {
            bool first = true;
            for (const auto& infoEntry : *extraInfo)
            {
                if (!first)
                {
                    trace.append(L",");
                }
                trace.append(L"\"");
                trace.append(infoEntry);
                trace.append(L"\"");

                first = false;
            }
        }
        DisplayReleaseMessage(trace.c_str());
    }

    return SetWarningContextLinkedListHead(context);
}


// Returns the thread's accumulated stowed exceptions so they can be used
// in a fail-fast call.
void GetStowedExceptionsForFailFast(
    _Outptr_result_buffer_maybenull_(*pcStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2** pppStowedExceptions,
    _Out_ unsigned int* pcStowedExceptions)
{
    *pppStowedExceptions = NULL;
    *pcStowedExceptions = 0;

    ErrorContext* errorContext = GetErrorContextLinkedListHead();

    if (errorContext)
    {
        MarkErrorContextFatal(errorContext);

        size_t warningsCount = 0;
        WarningContext* warningContext = GetWarningContextLinkedListHead();

        while (warningContext)
        {
            warningsCount++;
            warningContext = warningContext->next;
        }

        *pppStowedExceptions = static_cast<PSTOWED_EXCEPTION_INFORMATION_V2*>(
            HeapAlloc(GetProcessHeap(), 0, sizeof(**pppStowedExceptions) * (MAX_ERROR_CONTEXTS + warningsCount)));

        if (*pppStowedExceptions)
        {
            while (errorContext && *pcStowedExceptions < MAX_ERROR_CONTEXTS)
            {
                FillInStowedException(errorContext);
                (*pppStowedExceptions)[(*pcStowedExceptions)++] = &errorContext->stowedException;
                errorContext = errorContext->next;
            }

            // Include the optional WarningContexts' stowed exceptions which do not represent actual errors.
            if (warningsCount > 0)
            {
                warningContext = GetWarningContextLinkedListHead();

                while (warningContext)
                {
                    FillInStowedException(warningContext);
                    (*pppStowedExceptions)[(*pcStowedExceptions)++] = &warningContext->stowedException;
                    warningContext = warningContext->next;
                }
            }
        }
    }
}

// Trace an event to telemetry saying that we are about to failfast.
// If the trace provider is not registered we no-op.
void TraceForFailFast(HRESULT errorCode)
{
    if (g_hTraceProvider != nullptr && g_hTraceProvider->RegHandle != 0)
    {
        TraceLoggingWrite(g_hTraceProvider,
            "XamlFailFast",
            TraceLoggingInt32(errorCode, "ErrorCode"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    }
}

SuspendFailFastOnStowedException::SuspendFailFastOnStowedException() :
    m_previousSuspendState(g_suspendFailFastOnStowedException),
    m_previousSuspendCode(g_suspendFailFastOnSpecificStowedException)
{
    g_suspendFailFastOnStowedException = true;
    g_suspendFailFastOnSpecificStowedException = S_OK;
}

SuspendFailFastOnStowedException::SuspendFailFastOnStowedException(HRESULT codeToIgnore) :
    m_previousSuspendState(g_suspendFailFastOnStowedException),
    m_previousSuspendCode(g_suspendFailFastOnSpecificStowedException)
{
    g_suspendFailFastOnStowedException = false;
    g_suspendFailFastOnSpecificStowedException = codeToIgnore;
}

SuspendFailFastOnStowedException::~SuspendFailFastOnStowedException()
{
    g_suspendFailFastOnStowedException = m_previousSuspendState;
    g_suspendFailFastOnSpecificStowedException = m_previousSuspendCode;
}

static const XINT32 AgErrorCodeFlagBits = 0x88000000;

// AG Error codes can be encoded as HRESULTS by setting the high order bits.

HRESULT AgError(XUINT32 agErrorCode)
{
    return agErrorCode | AgErrorCodeFlagBits;
}

XUINT32 AgCodeFromHResult(HRESULT hr)
{
    return (hr & AgErrorCodeFlagBits) == AgErrorCodeFlagBits ? hr & ~AgErrorCodeFlagBits : 0;
}

std::function<decltype(RoFailFastWithErrorContextInternal2)> g_roFailFastMock;

void __stdcall FailFastWithStowedExceptions(
        HRESULT hrError,
        ULONG cStowedExceptions,
        _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[]) noexcept
{
    if (g_roFailFastMock)
    {
        g_roFailFastMock(hrError, cStowedExceptions, aStowedExceptionPointers);
        return;
    }

    typedef decltype(RoFailFastWithErrorContextInternal2)* RoFailFastWithErrorContextInternal2PtrType;

    // Ignore errors here.  If we fail to find the failfast function, we'll at least crash calling into null anyway.
    HMODULE module = ::LoadLibraryExW(L"ComBase.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    RoFailFastWithErrorContextInternal2PtrType roFailFastWithErrorContextInternal2Ptr = 
        reinterpret_cast<RoFailFastWithErrorContextInternal2PtrType>(::GetProcAddress(module, "RoFailFastWithErrorContextInternal2"));

   roFailFastWithErrorContextInternal2Ptr(hrError, cStowedExceptions, aStowedExceptionPointers);
}

// IXamlErrorTestHooks implementation class
class XamlErrorTestHooks : public Microsoft::WRL::RuntimeClass<IXamlErrorTestHooks>
{
    InspectableClass(L"Private.XamlErrorTestHooks", TrustLevel::BaseTrust);
    
public:
    XamlErrorTestHooks() {}

    // IXamlErrorTestHooks
    IFACEMETHOD_(void, SetRoFailFastMock)(std::function<decltype(RoFailFastWithErrorContextInternal2)> mock)
    {
        g_roFailFastMock = mock;
    }

    IFACEMETHOD_(void, ClearStowedExceptions)()
    {
        IGNOREHR(ErrorContextThreadDeinit());
    }
};

HRESULT CreateErrorHandlingTestHooks(_Outptr_opt_ IXamlErrorTestHooks** errorTestHooks)
{
    Microsoft::WRL::ComPtr<XamlErrorTestHooks> spErrorTestHooks = Microsoft::WRL::Make<XamlErrorTestHooks>();
    *errorTestHooks = spErrorTestHooks.Detach();
    return S_OK;
}
