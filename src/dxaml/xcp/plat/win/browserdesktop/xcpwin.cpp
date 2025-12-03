// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Microsoft.DirectManipulation.h"
#include "DirectManipulationService.h"

#include "XcpAllocation.h"

// Enable a bunch of host features (should be in host)

#include "hal.h"
#include "host.h"
#include "framecounter.h"
#include "corep.h"
#include "commonbrowserhost.h"
#include "asyncdownloadrequestmanager.h"

#include "winbrowserhost.h"

// Enable update checking (should be in host)

// Enable download requests (should be in host)

#include "download.h"
#include "DownloadRequest.h"
#include "windwnreq.h"

#include "xcpwin.tmh"

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "winuri.h"

#include "LoadLibraryAbs.h"

using namespace DirectUI;

#ifndef ERANGE
#define ERANGE          34
#endif

#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xffffffff
#endif

XUINT32 g_cCPUCores = 1;
XINT32  g_fCheckedCPUCores = FALSE;

#if XCP_MONITOR
CWinThreadMonitor *g_pThreadMonitor        = NULL;
#endif

#if DBG
// Assert behavior controlled by a registry setting
//
//0; Show Assert Dialog & DebugBreak
//1; No Show Assert Dialog & No DebugBreak
//2; No Show Assert Dialog & DebugBreak

XUINT8  g_AssertBehaviorFlags = 0;
#endif

#if DBG
XINT8  g_StressModeFlag = -1;
XUINT32  g_StressLeakThreshold = -1;
#endif

extern EncodedPtrWithDeleteAndgpsReset<CWindowsServices> theOnlyWinServices;

extern XHANDLE ghHeap;

/*
 * Union to facilitate converting from FILETIME to unsigned __int64
 */
typedef union {
    unsigned __int64 ft_scalar;
    FILETIME ft_struct;
} FT;

#if DBG
void SetSupressAssertDlgFlag()
{
    HKEY hPerfKey;
    HRESULT hr;
    DWORD dwType = 0;               // should be REG_DWORD
    DWORD buffer = 0;               // only need 4 bytes to store
    DWORD cbData = sizeof(buffer);

    // access the registry for the perf marker key

    IFC(RegOpenKeyEx(HKEY_CURRENT_USER,
        _T("Software\\Microsoft\\XCP\\DisableAssertDlg"),
        0,
        KEY_QUERY_VALUE,
        &hPerfKey));

    if (hPerfKey != NULL)
    {
        hr = RegQueryValueEx(hPerfKey, _T("enable"), NULL, &dwType, (byte*)&buffer, &cbData);

        if ((hr == ERROR_SUCCESS) && (buffer >= 1 && buffer <= 3))
        {
            g_AssertBehaviorFlags = static_cast<XUINT8>(buffer);
        }
    }

Cleanup:
    if (hPerfKey != NULL)
    {
        RegCloseKey(hPerfKey);
    }

    return;
}


#endif

void EmitHeapHandleExportEtwEvent()
{
    TraceExportHeapHandleInfo(reinterpret_cast<uint64_t>(ghHeap));
}

//------------------------------------------------------------------------
//
//  Function:   ObtainPlatformServices
//
//  Synopsis:
//      Returns a pointer to the global instance of the platform services.
//
//  Implementation notes:
//      This function is not thread safe.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
ObtainPlatformServices(
#if DBG
    _Outptr_ IPlatformServices **ppInterface,
    XUINT8 testMode
#else
    _Outptr_ IPlatformServices **ppInterface
#endif // #if DBG
)
{
    HRESULT hr = S_OK;
    *ppInterface = NULL;

    // A single global monitor used only in the highly unlikely case that there is no
    // thread local storage available.
#if XCP_MONITOR
    static  CWinThreadMonitor theonlyThreadMonitor;
#endif

    if (!gps.IsValid())
    {
        gps.Set(theOnlyWinServices.Get());

        // This registry key is useful for performance measurement. It will ensure that all XAML-framework
        // related heap allocations are kept in a private heap. This allows for WPA sessions to be captured
        // in such a way that the framework costs are easily separated out from components we don't own
        // (DComp RPC proxies, other DLL's private heaps, etc)
        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::UsePrivateHeap, true))
        {
            if(ghHeap != nullptr)
                XAML_FAIL_FAST();
            ghHeap = HeapCreate(0 /* options */, 0 /* initial size */, 0 /* max size */);
            EmitHeapHandleExportEtwEvent();
        }
        if (!ghHeap)
        {
            ghHeap = GetProcessHeap();
        }

#if XCP_MONITOR
        g_pThreadMonitor = &theonlyThreadMonitor;

#if DBG
        IFC(gps->XcpTraceMonitorInitialize(testMode));
        // Have to initialize the trace flags before calling this.
#else
        IFC(gps->XcpTraceMonitorInitialize());
#endif // #if DBG

#endif // #if XCP_MONITOR

#if RENDERCOUNTERS
        IFC(gps->XcpRenderCountersInitialize());
#endif // #if RENDERCOUNTERS

        gps->InitDebugTrace();
    }

    *ppInterface = gps.Get();

#if DBG
    SetSupressAssertDlgFlag();
#endif

    if (0)
    {
        // Forcing to keep the label in to support the other compilation variables
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetApplicationSingleton
//
//  Synopsis:
//      Gets a reference to the process-wide singleton control object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::GetApplicationSingleton(_Outptr_ IPALApplicationSingleton **ppSingleton)
{
    HRESULT hr = S_OK;

    IFC(CWinApplicationSingleton::Create(ppSingleton));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   UriCreate
//
//  Synopsis:
//      Creates a URI object from a string.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWindowsServices::UriCreate(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Outptr_ IPALUri **ppUri)
{
    return CWinUriFactory::Create(cString, pString, ppUri);
}

//------------------------------------------------------------------------
//
//  Method:   PrintStringCchW
//
//  Synopsis:
//      UNICODE version of safe sprintf function
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CWindowsServices::PrintStringCchW(
_Out_writes_(cchDest) WCHAR *pszDest,
_In_ XUINT32 cchDest,
_In_z_ const WCHAR *pszFormat,
...)
{
    va_list  vargs;
    va_start(vargs, pszFormat);

    return ::StringCchVPrintfW(pszDest, cchDest, pszFormat, vargs);
}

//------------------------------------------------------------------------
//
//  Method:   PrintStringCchExW
//
//  Synopsis:
//      UNICODE version of safe sprintf function with extended return parameters
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CWindowsServices::PrintStringCchExW(
_Out_writes_(cchDest) WCHAR *pszDest,
_In_ XUINT32 cchDest,
_Outptr_opt_result_buffer_(*pcchRemaining) WCHAR **ppszDestEnd,
_Out_opt_ XUINT32 *pcchRemaining,
_In_ XUINT32 dwFlags,
_In_z_ const WCHAR *pszFormat,
...)
{
    va_list  vargs;
    va_start(vargs, pszFormat);

    size_t cchRemainingLocal = 0;
    size_t* pcchRemainingLocal = nullptr;

    if (pcchRemaining != nullptr)
    {
        pcchRemainingLocal = &cchRemainingLocal;
    }

    HRESULT hr = ::StringCchVPrintfExW(pszDest, cchDest, ppszDestEnd, pcchRemainingLocal, dwFlags, pszFormat, vargs);

    if (pcchRemaining != nullptr)
    {
        *pcchRemaining = static_cast<XUINT32>(*pcchRemainingLocal);
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   ThreadCreate
//
//  Synopsis:
//      Create a thread.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CWindowsServices::ThreadCreate(
    _Outptr_ IPALWaitable **ppThread,
    _In_ PALTHREADPFN pfn,
    _In_ XUINT32 cData,
    _In_reads_(cData) XUINT8 *pData
)
{
    CWinOSHandle *pThread = NULL;
    HANDLE  h;
    HRESULT hr;

    h = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) pfn, (void *) pData, 0, NULL);
    if (h == nullptr)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        IFC(hr);
    }

    pThread = new CWinOSHandle(h);

   *ppThread = static_cast<IPALWaitable *>(pThread);
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CreateWorkItemFactory
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CWindowsServices::CreateWorkItemFactory(
           _Outptr_ IPALWorkItemFactory **ppFactory)
{
    HRESULT hr = S_OK;
    CWinWorkItemFactory *pNewFactory = NULL;

    if (ppFactory == NULL)
        IFC(E_INVALIDARG);

    pNewFactory = new CWinWorkItemFactory();
    IFC(pNewFactory->Initialize());

    *ppFactory = pNewFactory;
    pNewFactory = NULL;

Cleanup:
    ReleaseInterface(pNewFactory);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   EventCreate
//
//  Synopsis:
//      Create a signable event.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CWindowsServices::EventCreate(
    _Outptr_ IPALEvent **ppEvent,
    _In_ XINT32 bSignaled,
    _In_ XINT32 bManual
)
{
    CWinEvent *pEvent = NULL;
    HANDLE  h;
    HRESULT hr;

    h = ::CreateEvent(NULL, (BOOL) bManual, (BOOL) bSignaled, NULL);
    if (NULL == h)
    {
        return E_FAIL;
    }

    pEvent = new CWinEvent(h);

   *ppEvent = static_cast<IPALEvent *>(pEvent);
    hr = S_OK;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   QueueCreate
//
//  Synopsis:
//      Create a queue object.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CWindowsServices::QueueCreate(_Outptr_ IPALQueue **ppQueue)
{
    HRESULT hr;

    CWinQueue *pQueue = new CWinQueue;

    IFC(pQueue->Initialize());
    *ppQueue = static_cast<IPALQueue *>(pQueue);
    pQueue = NULL;

Cleanup:
    delete pQueue;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   NamedEventCreate
//
//  Synopsis:
//      Create a named (cross-process) event object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::NamedEventCreate(
    _Outptr_ IPALEvent** ppEvent,
    PAL_IPCInitMode initMode,
    bool bInitialState,
    bool bManualReset,
    _In_ const xstring_ptr_view& strName,
    bool bReturnFailureIfCreationFailed)
{
    HRESULT hr = S_OK;
    HANDLE hEvent = nullptr;
    wchar_t eventName[_MAX_PATH];
    CWinEvent* pEvent = nullptr;

    IFCEXPECT(ppEvent);
    *ppEvent = nullptr;

    IFCEXPECT(!strName.IsNullOrEmpty());

    {
        DWORD processId = ::GetCurrentProcessId();
        DWORD threadId = ::GetCurrentThreadId();
        uint32_t cchRemaining = 0;

        IFC(PrintStringCchExW(
            STR_LEN_PAIR(eventName), nullptr, &cchRemaining, 0,
            L"%s.%d.%d", strName.GetBuffer(), processId, threadId));
    }

    if (InitModeCreateOnly == initMode || InitModeOpenOrCreate == initMode)
    {
        LPSECURITY_ATTRIBUTES pSecurityAttributes = nullptr;
        hEvent = CreateEvent(pSecurityAttributes, bManualReset, bInitialState, eventName);
        if (hEvent == NULL)
        {
            if (bReturnFailureIfCreationFailed)
            {
                IFCW32(FALSE);
            }
            else
            {
                goto Cleanup;
            }
        }
        else if (InitModeCreateOnly == initMode && ERROR_ALREADY_EXISTS == GetLastError())
        {
            if (bReturnFailureIfCreationFailed)
            {
                IFC(E_FAIL);
            }
            else
            {
                goto Cleanup;
            }
        }
    }
    else if (InitModeOpenOnly == initMode)
    {
        hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE /* bInheritHandle */, eventName);
        if (hEvent == NULL)
        {
            if (bReturnFailureIfCreationFailed)
            {
                IFCW32(FALSE);
            }
            else
            {
                goto Cleanup;
            }
        }
    }
    else
    {
        IFC(E_INVALIDARG);
    }

    pEvent = new CWinEvent(hEvent);

    // The CWinEvent object is now responsible for this.
    hEvent = nullptr;

    *ppEvent = pEvent;
    pEvent = nullptr;

Cleanup:
    if (hEvent != nullptr)
    {
        CloseHandle(hEvent);
    }
    if (pEvent != NULL)
    {
        IGNOREHR(pEvent->Close());
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   WaitForObjects
//
//  Synopsis:
//      Wait for one or more objects.
//
//------------------------------------------------------------------------

XUINT32
CWindowsServices::WaitForObjects(
    _In_ XUINT32 cWaitable,
    _In_reads_(cWaitable) IPALWaitable **ppWaitable,
    _In_ XINT32 bAll,
    _In_ XUINT32 nTimeout
)
{
    HANDLE  ah[16];
    HANDLE *ph = NULL;
    XUINT32 iWait = 0;

// See if we can copy the system handles to the array on the frame.

    if (cWaitable > 16)
    {
        ph = new HANDLE[cWaitable];
    }
    else
    {
        ph = ah;
    }

// Extract the handles from the objects.  For the Win32 version we know all the
// objects have the handles at the front.  Just grab and go.

    for (iWait = 0; iWait != cWaitable; iWait++)
    {
        ph[iWait] = ppWaitable[iWait]->GetHandle();
    }

    do
    {
        iWait = ::WaitForMultipleObjectsEx(
            static_cast<DWORD>(cWaitable) /* nCount */,
            const_cast<const HANDLE *>(ph) /* pHandles */,
            static_cast<BOOL>(bAll) /* bWaitAll */,
            static_cast<DWORD>(nTimeout) /* dwMilliseconds */,
            TRUE /* bAlertable */);
    }
    while (iWait == WAIT_IO_COMPLETION);

    static_assert(WAIT_OBJECT_0 == 0, "WAIT_OBJECT_0 is not zero.");

    // We'll return 0xffffffff to indicate timeout or unknown termination
    if (iWait < WAIT_OBJECT_0 + cWaitable)
    {
    }
    else if ((iWait >= WAIT_ABANDONED_0) && (iWait < WAIT_ABANDONED_0 + cWaitable))
    {
        iWait -= WAIT_ABANDONED_0;
    }
    else
    {
        iWait = XUINT32(~0);
    }

    if (cWaitable > 16)
        delete [] ph;

    return iWait;
}

//------------------------------------------------------------------------
//
//  Method:   ThreadGetPriority
//
//  Synopsis:
//      Returns the priority of the thread.  If no thread object is specified
//  it gives the priority for the current thread.
//
//------------------------------------------------------------------------
#define HTHREAD HANDLE

XINT32
CWindowsServices::ThreadGetPriority(_In_opt_ IPALWaitable *pThread)
{
    XINT32  nPriority;

#if !defined(_WIN32_WCE)
    nPriority = ::GetThreadPriority(pThread ? static_cast<CWinOSHandle *>(pThread)->GetHandle() : GetCurrentThread());
#else
    nPriority = ::GetThreadPriority(pThread ? static_cast<HTHREAD>(static_cast<CWinOSHandle *>(pThread)->GetHandle()) : GetCurrentThread());
#endif

// Convert the OS priority to a PAL priority

    if (nPriority < -2)
        return PAL_THREAD_PRIORITY_IDLE;
    else if (nPriority > 2)
        return PAL_THREAD_PRIORITY_REAL_TIME;
    else
        return nPriority;
}

//------------------------------------------------------------------------
//
//  Method:   ThreadSetPriority
//
//  Synopsis:
//      Attempts to set the priority of the thread.  If no thread object is
//  specified it attempts to set the priority for the current thread.
//
//------------------------------------------------------------------------

HRESULT
CWindowsServices::ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority)
{
    HRESULT hr = S_OK;
    BOOL bSuccess;

// Convert the PAL priority to an OS priority

    switch (nPriority)
    {
    case PAL_THREAD_PRIORITY_IDLE:
        nPriority = THREAD_PRIORITY_IDLE;
        break;

    case PAL_THREAD_PRIORITY_REAL_TIME:
        nPriority = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    default:
        break;
    }

#if !defined(_WIN32_WCE)
    bSuccess = ::SetThreadPriority(pThread ? static_cast<CWinOSHandle *>(pThread)->GetHandle() : GetCurrentThread(), nPriority);
#else
    bSuccess = ::SetThreadPriority(pThread ? static_cast<HTHREAD>(static_cast<CWinOSHandle *>(pThread)->GetHandle()) : GetCurrentThread(), nPriority);
#endif

    if(!bSuccess)
    {
        IFC(E_FAIL);
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   InitDebugTrace
//
//  Synopsis:
//      Declare this module as an ETW Provider for debugging purposes
//
//------------------------------------------------------------------------
void
CWindowsServices::InitDebugTrace()
{
    WPP_INIT_TRACING(L"Microsoft\\Windows_UI_XAML");
}

//------------------------------------------------------------------------
//
//  Method:   CleanupDebugTrace
//
//  Synopsis:
//      Deactivate ETW tracing for debugging purposes
//
//------------------------------------------------------------------------
void
CWindowsServices::CleanupDebugTrace()
{
    WPP_CLEANUP();
}

//------------------------------------------------------------------------
//
//  Method:   IsDebugTraceTypeActive
//
//  Synopsis:
//      Returns TRUE when calling DebugTrace with the same traceType
//      results in a debug output or ETW trace.
//      Returns FALSE when calling DebugTrace with the same traceType
//      is a no-op.
//
//------------------------------------------------------------------------
bool
CWindowsServices::IsDebugTraceTypeActive(
    _In_ XDebugTraceType traceType)
{
    if (traceType & XCP_TRACE_OUTPUT_MSG)
    {
        return true;
    }

    if (traceType & XCP_TRACE_WPP_FLAGS_MASK)
    {
        ULONG ulWppLevel = TRACE_LEVEL_INFORMATION;
        if (traceType & XCP_TRACE_VERBOSE)
        {
            ulWppLevel = TRACE_LEVEL_VERBOSE;
        }
        else if (traceType & XCP_TRACE_WARNING)
        {
            ulWppLevel = TRACE_LEVEL_WARNING;
        }
        else if (traceType & XCP_TRACE_ERROR)
        {
            ulWppLevel = TRACE_LEVEL_ERROR;
        }

        switch (traceType & XCP_TRACE_WPP_FLAGS_MASK)
        {
        case XCP_TRACE_COMMON:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, Common);
        case XCP_TRACE_DM_COMPOSITOR:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_Compositor);
        case XCP_TRACE_DM_INPUT_MANAGER:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_InputManager);
        case XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_InputManagerViewport);
        case XCP_TRACE_DM_PAL_SERVICE:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_PALService);
        case XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_PALServiceViewportEventHandler);
        case XCP_TRACE_DM_SCROLLVIEWER:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_ScrollViewer);
        case XCP_TRACE_DM_SCROLLCONTENTPRESENTER:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, DManip_ScrollContentPresenter);
        case XCP_TRACE_RESOURCELOADING:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, ResourceLoading);
        case XCP_TRACE_LISTVIEWBASEITEMCHROME:
            return WPP_LEVEL_FLAGS_ENABLED(ulWppLevel, ListViewBaseItemChrome);
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Method:   DebugTrace
//
//  Synopsis:
//      If traceType includes XCP_TRACE_OUTPUT_MSG:
//         Print the string on the debugger console.
//         If traceType also includes XCP_TRACE_PREPEND_OUTPUT_WITH_THREAD_ID,
//         prepend the string with ThreadID=xxxxx.
//         If traceType also includes XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID,
//         append the string with ThreadID=xxxxx.
//      If traceType includes any bit in XCP_TRACE_WPP_FLAGS_MASK:
//         Emit an ETW trace if the level and flags are enabled.
//         The level is TRACE_LEVEL_INFORMATION by default. It is TRACE_LEVEL_VERBOSE
//         if traceType includes the XCP_TRACE_VERBOSE bit.
//         The ETW flag depends on the bit set within the XCP_TRACE_WPP_FLAGS_MASK mask.
//      For performance reasons, it is assumed that IsDebugTraceTypeActive is
//      called prior to calling DebugTrace to check if the flag/level combination is
//      enabled, in case an ETW tracing is the only request. Attempting to emit an ETW
//      trace for a disabled combination is not going to cause a failure, it's just
//      wasting CPU.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::DebugTrace(
    _In_ XDebugTraceType traceType,
    _In_z_ const WCHAR *pFormattedMsg, ...)
{
    HRESULT hr = S_OK;
    va_list args;
    va_start(args, pFormattedMsg);
    IFC(DebugTrace(traceType, pFormattedMsg, args));
    va_end(args);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CWindowsServices::DebugTrace(
    _In_ XDebugTraceType traceType,
    _In_z_ const WCHAR *pFormattedMsg,
    _In_ va_list args)
{
    HRESULT hr = S_OK;
    WCHAR msg[320];
    WCHAR msg2[336];
    bool fAddCarriageReturn = true;
    CHAR* szMsg = NULL;
    ULONG ulWppLevel = TRACE_LEVEL_INFORMATION;
    IPlatformUtilities *pUtilities = NULL;

    IFC(GetPlatformUtilities(&pUtilities));
    ASSERT(pUtilities);
    pUtilities->vXswprintf_s(msg, ARRAY_SIZE(msg), pFormattedMsg, args);

    if (traceType & XCP_TRACE_WPP_FLAGS_MASK)
    {
        // Emit the ETW trace
        szMsg = ::WideCharToChar(320, msg);
        if (szMsg)
        {
            if (traceType & XCP_TRACE_VERBOSE)
            {
                ulWppLevel = TRACE_LEVEL_VERBOSE;
            }
            else if (traceType & XCP_TRACE_WARNING)
            {
                ulWppLevel = TRACE_LEVEL_WARNING;
            }
            else if (traceType & XCP_TRACE_ERROR)
            {
                ulWppLevel = TRACE_LEVEL_ERROR;
            }
            switch (traceType & XCP_TRACE_WPP_FLAGS_MASK)
            {
            case XCP_TRACE_COMMON:
                DoTraceLevelMessage(ulWppLevel, Common, "%s", szMsg);
                break;
            case XCP_TRACE_DM_COMPOSITOR:
                DoTraceLevelMessage(ulWppLevel, DManip_Compositor, "%s", szMsg);
                break;
            case XCP_TRACE_DM_INPUT_MANAGER:
                DoTraceLevelMessage(ulWppLevel, DManip_InputManager, "%s", szMsg);
                break;
            case XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT:
                DoTraceLevelMessage(ulWppLevel, DManip_InputManagerViewport, "%s", szMsg);
                break;
            case XCP_TRACE_DM_PAL_SERVICE:
                DoTraceLevelMessage(ulWppLevel, DManip_PALService, "%s", szMsg);
                break;
            case XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER:
                DoTraceLevelMessage(ulWppLevel, DManip_PALServiceViewportEventHandler, "%s", szMsg);
                break;
            case XCP_TRACE_DM_SCROLLVIEWER:
                DoTraceLevelMessage(ulWppLevel, DManip_ScrollViewer, "%s", szMsg);
                break;
            case XCP_TRACE_DM_SCROLLCONTENTPRESENTER:
                DoTraceLevelMessage(ulWppLevel, DManip_ScrollContentPresenter, "%s", szMsg);
                break;
            case XCP_TRACE_RESOURCELOADING:
                DoTraceLevelMessage(ulWppLevel, ResourceLoading, "%s", szMsg);
                break;
            case XCP_TRACE_LISTVIEWBASEITEMCHROME:
                DoTraceLevelMessage(ulWppLevel, ListViewBaseItemChrome, "%s", szMsg);
                break;
            }
        }
    }

    if (traceType & XCP_TRACE_OUTPUT_MSG)
    {
        // Output the debug string
        if (traceType & XCP_TRACE_NO_NEW_LINE_IN_OUTPUT_MSG)
        {
            fAddCarriageReturn = false;
        }
        if (traceType & (XCP_TRACE_PREPEND_OUTPUT_WITH_THREAD_ID | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID))
        {
            if (traceType & XCP_TRACE_PREPEND_OUTPUT_WITH_THREAD_ID)
            {
                pUtilities->Xswprintf_s(msg2, ARRAY_SIZE(msg2), fAddCarriageReturn ? L"ThreadID=%d. %s\n" : L"ThreadID=%d. %s", GetCurrentThreadId(), msg);
            }
            else
            {
                pUtilities->Xswprintf_s(msg2, ARRAY_SIZE(msg2), fAddCarriageReturn ? L"%s ThreadID=%d.\n" : L"%s ThreadID=%d.", msg, GetCurrentThreadId());
            }
            OutputDebugString(msg2);
        }
        else if (fAddCarriageReturn)
        {
            pUtilities->Xswprintf_s(msg2, ARRAY_SIZE(msg2), L"%s\n", msg);
            OutputDebugString(msg2);
        }
        else
        {
            OutputDebugString(msg);
        }
    }

Cleanup:
    delete [] szMsg;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   DebugOutputSzNoEndl
//
//  Synopsis:
//      Print the string on the debugger console without \n. Unlike
//      DebugOutputSz(), this works in any build and avoids the problem
//      of having a context switch before outputting \n.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::DebugOutputSzNoEndl(_In_z_ const WCHAR *pFormattedMsg, ...)
{
    va_list args;
    WCHAR buf[256];
    IPlatformUtilities *pUtilities = NULL;
    HRESULT hr = S_OK;

    IFC(GetPlatformUtilities(&pUtilities));

    va_start(args, pFormattedMsg);
    pUtilities->vXswprintf_s(buf, ARRAY_SIZE(buf), pFormattedMsg, args);
    va_end(args);

    OutputDebugString(buf);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   DebugOutputSz
//
//  Synopsis:
//      Print the string on the debugger console.
//
//------------------------------------------------------------------------
void CWindowsServices::DebugOutputSz(
    _In_z_ const WCHAR *pMessage
)
{
    LOG_INFO_EX(pMessage);
    OutputDebugString(L"\n");
}

#if DBG
struct MessageBoxThreadData
{
    WCHAR pszDescription[4096];
    bool fResult;
};

//------------------------------------------------------------------------
//
//  Method:  CWindowsServices::DebugAssertMessageBox
//
//  Synopsis:
//
//      Used during debugging to report a failed assert and
//  ask whether to break into the debugger.
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable:26023)
_Check_return_ bool
CWindowsServices::DebugAssertMessageBox(
    _In_z_ const WCHAR *pCondition,
    _In_z_ const WCHAR *pFile,
    XUINT32 nLine)
{
    MessageBoxThreadData description;
    // force a DebugBreak on error
    // Set AssertBehaviorFlags to 1 or 3 to override this

    bool fDebugBreak = true;
    description.fResult = fDebugBreak;

    // Check for a debugger
    if (!IsDebuggerPresent())
    {
        StringCchPrintf(
            description.pszDescription,
            ARRAY_SIZE(description.pszDescription),
            L"Assertion: %s\n\n... at %s line %d\n\nPlease hit YES, attach a debugger and file a bug with the callstack.\nPress 'Yes' to call DebugBreak(), 'No' to ignore and continue.",
            pCondition,
            pFile,
            nLine
        );

        // Default flags cause Message Box to be shown asking the dev if
        // they want to attach a debugger
        if (g_AssertBehaviorFlags == 0)
        {
            // TODO:  Show Immersive Message Box
            fDebugBreak = description.fResult;
        }

        // g_AssertBehaviorFlags == 2 -- Don't show message box, do DebugBreak

        // Don't DebugBreak without debugger attached
        if (g_AssertBehaviorFlags == 1)
        {
            fDebugBreak = FALSE;
        }

    }
    else if(IsStressModeEnabled())
    {
        // Never break on asserts if a debugger is attached and we're in stress mode.
        fDebugBreak = FALSE;
    }

    // Output assertion message to debug output
    StringCchPrintf(
        description.pszDescription,
        ARRAY_SIZE(description.pszDescription),
        L"Assertion: %s at %s (%d)",
        pCondition,
        pFile,
        nLine
    );

    DebugOutputSz(description.pszDescription);
    TESTSignalASSERT(description.pszDescription);


    // Don't do DebugBreak() here, because the WER/Watson mechanisms won't
    // go far enough up the stack in assigning "blame", and all asserts
    // will look the same.  Instead, use return value of this function to
    // propagate back up the stack prior to using FAIL_ASSERT_HERE() up
    // closer to the asserting code, so that WER/Watson will know what to
    // do with the assert, and so the assert gets to do normal "ignore"
    // mechanisms with the debugger, etc.  But only do that stuff if this
    // function says so, in order that g_AssertBehaviorFlags can work.

    return fDebugBreak;
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
//  Method:  CWindowsServices::TESTSignalMemLeak
//
//  Synopsis:
//
//      This is very specific for the test code. this allows the
//      test to catch memory leak errors with out poping the dialog
//
//------------------------------------------------------------------------
bool CWindowsServices::TESTSignalMemLeak()
{
    HANDLE hTestEvent = NULL;

    hTestEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"XAMLTestNativeLeak");

    if (hTestEvent)
    {
        SetEvent(hTestEvent);
        CloseHandle(hTestEvent);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Function: TESTSignalManagedMemLeak
//
//  Synopsis: Signals the unit tests that there is a managed memory leak and returns
//                 whether or not to show an Assert dialog on the managed side.
//
//------------------------------------------------------------------------
void CWindowsServices::TESTSignalManagedMemLeak(_Out_ bool * showAssert)
{
    HANDLE hTestEvent = NULL;

    hTestEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"XAMLTestManagedLeak");
    if (hTestEvent)
    {
        SetEvent(hTestEvent);
        CloseHandle(hTestEvent);
        *showAssert = FALSE;
    }
    else
    {
        *showAssert = TRUE;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis: Signals the unit tests that was an ASSERT and the test failed
//
//------------------------------------------------------------------------

void CWindowsServices::TESTSignalASSERT(_In_z_ const WCHAR *pString)
{
    HANDLE hTestEvent = NULL;
    // TODO -- Communicate ASSERT string to test harness

    hTestEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"XAMLTestASSERT");

    if (hTestEvent)
    {
        SetEvent(hTestEvent);
        CloseHandle(hTestEvent);
    }
}

void CWindowsServices::TESTSignalReferenceTrackerLeak()
{
    HANDLE hTestEvent = NULL;

    hTestEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"XAMLTestReferenceTrackerLeak");

    if (hTestEvent)
    {
        SetEvent(hTestEvent);
        CloseHandle(hTestEvent);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis: Checks if EnableStressMode is set in registry.
//  Used for stress runs with CHK builds.
//
//------------------------------------------------------------------------
bool CWindowsServices::IsStressModeEnabled()
{
    LONG result = 0;
    HKEY hXamlKey = NULL;
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);

    if(g_StressModeFlag != -1)
    {
        goto Cleanup;
    }

    g_StressModeFlag = FALSE; // default value

    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        XAML_ROOT_KEY,
        0,
        KEY_QUERY_VALUE,
        &hXamlKey);

    if (ERROR_SUCCESS != result)
    {
        goto Cleanup;
    }

    result = RegQueryValueEx(
        hXamlKey,
        L"EnableStressMode",
        NULL,
        &dwType,
        (LPBYTE) &dwValue,
        &cbValue);

    if (ERROR_SUCCESS != result || REG_DWORD != dwType)
    {
        goto Cleanup;
    }

    g_StressModeFlag = (dwValue == 1);

Cleanup:
    if(hXamlKey != NULL)
        RegCloseKey(hXamlKey);

    return !!g_StressModeFlag;
}

//------------------------------------------------------------------------
//
//  Synopsis: Gets threshold for reporting leaks in stress.
//  Reads StressLeakThreshold value from registry.
//  Used for stress runs with CHK builds.
//
//------------------------------------------------------------------------
XUINT32 CWindowsServices::GetStressLeakThreshold()
{
    LONG result = 0;
    HKEY hXamlKey = NULL;
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);

    if(g_StressLeakThreshold != -1)
    {
        goto Cleanup;
    }

    g_StressLeakThreshold = 512; // default value

    result = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        XAML_ROOT_KEY,
        0,
        KEY_QUERY_VALUE,
        &hXamlKey);

    if (ERROR_SUCCESS != result)
    {
        goto Cleanup;
    }

    result = RegQueryValueEx(
        hXamlKey,
        L"StressLeakThreshold",
        NULL,
        &dwType,
        (LPBYTE) &dwValue,
        &cbValue);

    if (ERROR_SUCCESS != result || REG_DWORD != dwType)
    {
        goto Cleanup;
    }

    g_StressLeakThreshold = dwValue;

Cleanup:
    if(hXamlKey != NULL)
        RegCloseKey(hXamlKey);

    return g_StressLeakThreshold;
}

#endif // #if DBG


#if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Method:   CWindowsServices::YieldSlice
//
//  Synopsis:
//
//      Called when the monitor buffer is full to allow the monitoring
//      process to make space in the buffer.
//
//------------------------------------------------------------------------

void CWindowsServices::YieldSlice()
{
    Sleep(1);
}

#endif // #if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Method:   CreateClock
//
//  Synopsis:
//      Create a PAL clock
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWindowsServices::CreateClock(_Outptr_ IPALClock **ppIClock)
{
    HRESULT hr = S_OK;
    CWinClock *pClock;

    IFC(CWinClock::Create(&pClock));
    *ppIClock = pClock;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetCPUTime
//
//  Synopsis:
//      Returns the up time of the CPU in nanoseconds
//
//------------------------------------------------------------------------

XUINT64
CWindowsServices::GetCPUTime()
{
    if (m_eFrequency == 0.0)
    {
        LARGE_INTEGER liFreq = {};

        QueryPerformanceFrequency(&liFreq);

        m_eFrequency = double(liFreq.QuadPart) / 1000000000.0;
    }

    LARGE_INTEGER li = {};

    QueryPerformanceCounter(&li);

    return XUINT64(double(li.QuadPart) / m_eFrequency);
}

//------------------------------------------------------------------------
//
//  Method:   GetCPUMilliseconds
//
//  Synopsis:
//      Returns the up time of the CPU in miliseconds
//
//------------------------------------------------------------------------
XUINT64
CWindowsServices::GetCPUMilliseconds()
{
    return GetTickCount64();
}

XUINT32 ComputeCPUCoreCount();

//------------------------------------------------------------------------
//
//  Method:   CWindowsServices::GetNumberOfCPUCores
//
//  Synopsis:
//      Get the number of cores on the machine
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWindowsServices::GetNumberOfCPUCores(
    _Out_ XUINT32 *pcProcessorCores
    )
{
    if (!g_fCheckedCPUCores)
    {
        // It's possible this initialization condition is called simultaneously from multiple
        // threads in rare conditions.  However, if that occurs, then we would call
        // ::ComputeCPUCoreCount() twice which would be benign.

        g_cCPUCores = ::ComputeCPUCoreCount();
        g_fCheckedCPUCores = TRUE;
    }
    *pcProcessorCores = g_cCPUCores;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CallCoInitializeMTA
//
//  Synopsis:
//      Initialize COM for MTA model
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::CallCoInitializeMTA()
{
    return ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

//------------------------------------------------------------------------
//
//  Method:   CallCoUninitialize
//
//  Synopsis:
//      Uninitialize COM
//
//------------------------------------------------------------------------
void CWindowsServices::CallCoUninitialize()
{
    ::CoUninitialize();
}

//------------------------------------------------------------------------
//
//  Method:   XcpPerfMarker
//
//  Synopsis:
//      takes a performance marker, parses part of it, and then posts
//      the relevant data to the listening processes
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::XcpPerfMarker(_In_ XUINT64 lMarker)
{
    HRESULT hr = S_OK;

    // a non-NULL handle means that our process is being monitored by a performance application
    if (m_hPerfMonitor)
    {
        // NOTE: this will get ignored if the browser host is listening as well.
        hr = SendXcpPerfMarker(lMarker, m_hPerfMonitor);
    }

    // a non-NULL handle means that we are hosted inside of a browserhost that's listening for render events
    if (m_hBrowserControl)
    {
        hr = SendXcpPerfMarker(lMarker, m_hBrowserControl);
    }

    return hr;
 }

_Check_return_ HRESULT CWindowsServices::XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, _In_ WCHAR* szMarkerInfo)
{
    HRESULT hr = S_OK;

    // a non-NULL handle means that our process is being monitored by a performance application
    if (m_hPerfMonitor)
    {
        // NOTE: this will get ignored if the browser host is listening as well.
        hr = SendXcpPerfMarker(lMarker, m_hPerfMonitor);
    }

    // a non-NULL handle means that we are hosted inside of a browserhost that's listening for render events
    if (m_hBrowserControl)
    {
        hr = SendXcpPerfMarker(lMarker, m_hBrowserControl);
    }

    return hr;
}

_Check_return_ HRESULT CWindowsServices::XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, _In_ XDWORD nMarkerInfo)
{
    HRESULT hr = S_OK;

    // a non-NULL handle means that our process is being monitored by a performance application
    if (m_hPerfMonitor)
    {
        // NOTE: this will get ignored if the browser host is listening as well.
        hr = SendXcpPerfMarker(lMarker, m_hPerfMonitor);
    }

    // a non-NULL handle means that we are hosted inside of a browserhost that's listening for render events
    if (m_hBrowserControl)
    {
        hr = SendXcpPerfMarker(lMarker, m_hBrowserControl);
    }

    return hr;
}


//------------------------------------------------------------------------
//
//  Method:   SendXcpPerfMarker
//
//  Synopsis:
//      takes a performance marker, parses part of it, and then posts
//      the relevant data to the listening process
//
//  Implementation details:
//      PostMessage is lightweight and asynchronous, a pair of important
//      qualities that are difficult to find in other cross-process
//      communication technologies. The restriction is that we are limited
//      to the size of WPARAM and LPARAM (both 32 or 64 bit, depending on
//      chip architecture) so we chop the value into two 32-bit pieces
//      and post it to the monitor window
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::SendXcpPerfMarker(_In_ XUINT64 lMarker, _In_ HWND hTargetWindow)
{
    HRESULT hr = E_FAIL;

    LARGE_INTEGER lMarkerData;

// assign to LARGE_INTEGER so that we can easily break into halves
    lMarkerData.QuadPart = lMarker;

// now send off our message
    IFCW32(PostMessage(hTargetWindow, WM_XCPPERFDATA, lMarkerData.HighPart, lMarkerData.LowPart));

// see if we need to post additional data
    if (lMarker & XCP_PMK_EVENT)
    {
        IFCW32(QueryPerformanceCounter(&lMarkerData));
        IFCW32(PostMessage(hTargetWindow, WM_XCPPERFDATA, lMarkerData.HighPart, lMarkerData.LowPart));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:     PerfMarkersEnabled
//
//  Synopsis:   performance measurement enabled?
//
//------------------------------------------------------------------------
PerfMarkerType CWindowsServices::PerfMarkersEnabled()
{
    if (!m_perfCheckCompleted)
    {
        HKEY hPerfKey=NULL;
        HRESULT hr;
        DWORD dwType = 0;               // should be REG_DWORD
        DWORD buffer = 0;               // only need 4 bytes to store
        DWORD cbData = sizeof(buffer);
        XUINT32 m_perfMarkersEnabled = FALSE;

        // access the registry for the perf marker key
        IFC(RegOpenKeyEx(HKEY_CURRENT_USER,
                         _T("Software\\Microsoft\\XCP\\Performance"),
                         0,
                         KEY_QUERY_VALUE,
                         &hPerfKey));

        if (hPerfKey != NULL)
        {
            RegQueryValueEx(hPerfKey, _T("enabled"), NULL, &dwType, (byte*)&buffer, &cbData);
            RegCloseKey(hPerfKey);
            m_hPerfMonitor = FindWindow(PERF_WINDOWCLASSNAME, NULL);
            m_perfMarkersEnabled |= buffer;
            if ( m_perfMarkersEnabled )
            {
                m_perfMarkerType = PerfMarkerEnabled;
                // Resetting this flag to ensure the browser control registry check
                // doesn't get the pervious value.
                m_perfMarkersEnabled = FALSE;
            }

            // post the HPC frequency
            LARGE_INTEGER lFrequencyData;
            lFrequencyData.QuadPart = XCP_PMK_FREQ;
            IFCW32(PostMessage(m_hPerfMonitor, WM_XCPPERFDATA, lFrequencyData.HighPart, lFrequencyData.LowPart));
            IFCW32(QueryPerformanceFrequency(&lFrequencyData));
            IFCW32(PostMessage(m_hPerfMonitor, WM_XCPPERFDATA, lFrequencyData.HighPart, lFrequencyData.LowPart));
        }

        // access the registry for the browser control key
        IFC(RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("Software\\Microsoft\\XCP\\BrowserControl"),
                     0,
                     KEY_QUERY_VALUE,
                     &hPerfKey));

        if (hPerfKey != NULL)
        {
            cbData = sizeof(buffer);
            RegQueryValueEx(hPerfKey, _T("enabled"), NULL, &dwType, (byte*)&buffer, &cbData);
            RegCloseKey(hPerfKey);
            m_hBrowserControl = FindWindow(BROWSERCONTROL_WINDOWCLASSNAME, NULL);
            m_perfMarkersEnabled |= buffer;
            if (m_perfMarkersEnabled)
            {
                m_perfMarkerType = (PerfMarkerType)(m_perfMarkerType + PerfBrowserControlMarkerEnabled);
            }

            // post the HPC frequency
            LARGE_INTEGER lFrequencyData;
            lFrequencyData.QuadPart = XCP_PMK_FREQ;
            IFCW32(PostMessage(m_hBrowserControl, WM_XCPPERFDATA, lFrequencyData.HighPart, lFrequencyData.LowPart));
            IFCW32(QueryPerformanceFrequency(&lFrequencyData));
            IFCW32(PostMessage(m_hBrowserControl, WM_XCPPERFDATA, lFrequencyData.HighPart, lFrequencyData.LowPart));
        }

  Cleanup:
        m_perfCheckCompleted = true;
    }
    return m_perfMarkerType;
}


//------------------------------------------------------------------------
//
//  Method:   CreateIPALStreamFromIPALMemory
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
HRESULT CWindowsServices::CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Outptr_ IPALStream **ppPALStream)
{
    HRESULT hr = S_OK;
    CWinDataStream *pStream = NULL;
    CWinDataStreamBuffer *pWinDataStreamBuffer = NULL;

    IFCPTR(pPALMemory);
    IFCPTR(ppPALStream);

    IFC(CWinDataStreamBuffer::CreateFromIPalMemory(pPALMemory, &pWinDataStreamBuffer));
    IFC(CWinDataStream::Create(pWinDataStreamBuffer, &pStream));

    *ppPALStream = pStream;

    pStream = NULL;

Cleanup:
    ReleaseInterface(pWinDataStreamBuffer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   BrowserHostCreate
//
//  Synopsis:
//      Create a Browser host object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::BrowserHostCreate(
    _In_ IXcpHostSite *pSite,
    _In_ IXcpDispatcher *pDispatcher,
    _Outptr_ IXcpBrowserHost **ppHost
    )
{
    return CXcpBrowserHost::Create(pSite, pDispatcher, ppHost);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether DirectManipulation is supported by attempting to create a direct
//      manipulation manager.
//
//  Notes:
//      Some windows builds do not support DirectManipulation.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CWindowsServices::IsDirectManipulationSupported(_Out_ bool &isDirectManipulationSupported)
{
    if (!m_hasCheckedForDirectManipulationSupport)
    {
        IDirectManipulationManager *pDMManager = NULL;

        HMODULE hmodDManip = LoadLibraryExWAbs(L"Microsoft.DirectManipulation.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        IFCW32_RETURN(hmodDManip);

        wrl::ComPtr<IClassFactory> directManipulationFactory;
        IFCFAILFAST(DirectManipulationHelper::GetDirectManipulationManagerFactory(hmodDManip, &directManipulationFactory));

        m_isDirectManipulationSupported = TRUE;
        if (FAILED(directManipulationFactory->CreateInstance(nullptr, IID_PPV_ARGS(&pDMManager))))
        {
            m_isDirectManipulationSupported = FALSE;
        }

        m_hasCheckedForDirectManipulationSupport = TRUE;
        ReleaseInterface(pDMManager);
    }

    isDirectManipulationSupported = m_isDirectManipulationSupported;
    RRETURN(S_OK);
}

#pragma prefast( pop )

//------------------------------------------------------------------------
//
//  Method:   GetDirectManipulationService
//
//  Synopsis:
//    Returns an IPALDirectManipulationService implementation for
//    interacting with the Windows8 DirectManipulation APIs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetDirectManipulationService(
    std::shared_ptr<DirectManipulationServiceSharedState> sharedState,
    _Outptr_ IPALDirectManipulationService** ppDirectManipulationService
    )
{
    HRESULT hr = S_OK;
    CDirectManipulationService* pDirectManipulationService = NULL;

    IFCPTR(ppDirectManipulationService);
    *ppDirectManipulationService = NULL;

    IFC(CDirectManipulationService::Create(std::move(sharedState), &pDirectManipulationService));
    *ppDirectManipulationService = (IPALDirectManipulationService*) pDirectManipulationService;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetInputPaneInteraction
//
//  Synopsis:
//      Obtain a InputPane interaction
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetInputPaneInteraction(
    _In_ IXcpInputPaneHandler *pInputPaneHandler,
    _Outptr_ IPALInputPaneInteraction** ppInputPaneInteraction)
{
    HRESULT hr = S_OK;
    CInputPaneInteractionHelper *pInputPaneInteraction = NULL;

    IFCPTR(pInputPaneHandler);
    IFCPTR(ppInputPaneInteraction);
    *ppInputPaneInteraction = NULL;

    IFC(CInputPaneInteractionHelper::Create(pInputPaneHandler, &pInputPaneInteraction));
    *ppInputPaneInteraction = (IPALInputPaneInteraction*)pInputPaneInteraction;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetPlatformUtilities
//
//  Synopsis:
//      Obtain a singleton platform Utils
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetPlatformUtilities(
    _Outptr_ IPlatformUtilities** ppInterface)
{
    *ppInterface = CWinPlatformUtilities::getInstance();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GenerateSecurityToken
//
//  Synopsis:
//     Create a token with an unpredictable value which is
//     different for each module in a process.  Combine a number of sources
//     of randomness.
//
//------------------------------------------------------------------------
XUINT32
CWindowsServices::GenerateSecurityToken()
{
    FT systime={0};
    LARGE_INTEGER perfctr = {0};

    XUINT32 cookie = 0;

    ::GetSystemTimeAsFileTime(&systime.ft_struct);
    cookie = systime.ft_struct.dwLowDateTime;
    cookie ^= systime.ft_struct.dwHighDateTime;

    cookie ^= ::GetCurrentProcessId();
    cookie ^= ::GetCurrentThreadId();
    cookie ^= GetTickCount();

    QueryPerformanceCounter(&perfctr);
    cookie ^= perfctr.LowPart;
    cookie ^= perfctr.HighPart;

    return cookie;
}


//------------------------------------------------------------------------
//
//  Method:   CWinClock::Create
//
//  Synopsis:
//      Create a clock object
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinClock::Create(_Outptr_ CWinClock **ppClock)
{
    HRESULT hr = S_OK;

   *ppClock = new CWinClock();

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CWinClock::CWinClock
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------

CWinClock::CWinClock()
{
    QueryPerformanceCounter(&m_lTimeStart);
    QueryPerformanceFrequency(&m_lFreq);
}

//------------------------------------------------------------------------
//
//  Method:   CWinClock::GetAbsoluteTimeInSeconds
//
//  Synopsis:
//      Return the absolute time in seconds.  The base is arbitrary, so
//      only deltas of absolute time can be used.
//
//------------------------------------------------------------------------

XDOUBLE
CWinClock::GetAbsoluteTimeInSeconds()
{
    double rTime;
    LARGE_INTEGER lTimeEnd = {0};

    QueryPerformanceCounter(&lTimeEnd);

    rTime = XDOUBLE(lTimeEnd.QuadPart-m_lTimeStart.QuadPart)/XDOUBLE(m_lFreq.QuadPart);

    return rTime;
}

#if XCP_MONITOR

    //------------------------------------------------------------------------
    //
    //  Method:   CWinThreadMonitor::Suspend
    //
    //  Synopsis:
    //      Record clock count in m_clocksAtLastSuspend
    //
    //------------------------------------------------------------------------

    void CWinThreadMonitor::Suspend()
    {
    #if defined(_X86_)
        XUINT64 clocksNow;
        _asm  _emit 0FH
        _asm  _emit 31H
        _asm  mov dword ptr clocksNow+4,edx
        _asm  mov dword ptr clocksNow,eax
        m_clocksAtLastSuspend = clocksNow;
    #else
        QueryPerformanceCounter((LARGE_INTEGER*)&m_clocksAtLastSuspend);
    #endif
    }


    //------------------------------------------------------------------------
    //
    //  Method:   CWinThreadMonitor::Resume
    //
    //  Synopsis:
    //      Update m_clocksWhileSuspended with elapsed clocks since last suspend.
    //
    //------------------------------------------------------------------------

    void CWinThreadMonitor::Resume()
    {
        XUINT64 clocksNow;
    #if defined(_X86_)
        _asm  _emit 0FH
        _asm  _emit 31H
        _asm  mov dword ptr clocksNow+4,edx
        _asm  mov dword ptr clocksNow,eax
    #else
        QueryPerformanceCounter((LARGE_INTEGER*)&clocksNow);
    #endif
        m_clocksWhileSuspended += clocksNow - m_clocksAtLastSuspend;
    }


    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::GetThreadMonitor
    //
    //  Synopsis:
    //
    //      Obtains the address of the timing structure for the current thread.
    //      Creates a new one if none has yet been created.
    //      Also implictly performs a Suspend() on the thread monitor.
    //
    //------------------------------------------------------------------------

    // Thread local storage: The monitor process uses thread local storage to
    // keep per-thread performance variables, allowing clocks used during normal
    // processing and clocks used by monitor processing to be accounted correctly
    // in a multi-threaded process.

    XUINT32 g_dwThreadLocalStorageIndex = TLS_OUT_OF_INDEXES;


    IThreadMonitor *CWindowsServices::GetThreadMonitor()
    {
        XUINT64 clocksAtSuspend;
        #if defined(_X86_)
            _asm  _emit 0FH
            _asm  _emit 31H
            _asm  mov dword ptr clocksAtSuspend+4,edx
            _asm  mov dword ptr clocksAtSuspend,eax
        #else
            QueryPerformanceCounter((LARGE_INTEGER*)&clocksAtSuspend);
        #endif

        CWinThreadMonitor *pThreadMonitor;
        if (g_dwThreadLocalStorageIndex == TLS_OUT_OF_INDEXES)
        {
            // No thread local storage available
            pThreadMonitor = g_pThreadMonitor;

            if (g_pThreadMonitor->m_clocksWhileSuspended == 0)
            {
                // First use. Set clocks while suspended to exclude all
                // prior clocks from timing data.
                pThreadMonitor->m_clocksWhileSuspended = clocksAtSuspend;
            }
        }
        else
        {
            pThreadMonitor = (CWinThreadMonitor*) TlsGetValue(g_dwThreadLocalStorageIndex);
            if (!pThreadMonitor)
            {
                // Allocate timing block for this thread. (We'll leave thread
                // termination to clear it up).
                pThreadMonitor = new (1,1) CWinThreadMonitor;
                if (pThreadMonitor)
                {
                    // Since this is the first usage of the thread monitor, exclude
                    // all prior clocks from timing data.
                    pThreadMonitor->m_clocksWhileSuspended = clocksAtSuspend;
                    pThreadMonitor->m_threadId             = ::GetCurrentThreadId();

                    TlsSetValue(g_dwThreadLocalStorageIndex, pThreadMonitor);
                }

            }
        }
        if (pThreadMonitor)
        {
            pThreadMonitor->m_clocksAtLastSuspend = clocksAtSuspend;
        }
        return pThreadMonitor;
    }


    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::InitializeThreadMonitoring
    //
    //  Synopsis:
    //
    //      Prepares the thread monitoring global variables.
    //
    //------------------------------------------------------------------------

    void CWindowsServices::InitializeThreadMonitoring()
    {
        // Allocate an index for thread local storage.
        g_dwThreadLocalStorageIndex = TlsAlloc();

        // Note: if TlsAlloc fails, we'll just go ahead using g_Timing - one
        // copy for all threads. This works fine as long as no processor
        // or thread switch happens between Suspend() and Resume().
        // Unfortunately this is quite possible.
        // Fortunately TlsAlloc is unlikely to fail.

        if (g_dwThreadLocalStorageIndex == TLS_OUT_OF_INDEXES)
        {
            #if DBG
                DebugOutputSz(L"Couldn't allocate thread local storage for timing data,\n");
                DebugOutputSz(L"so timing data will likely be broken if there are multiple threads running.\n");
            #endif
            g_pThreadMonitor->m_clocksAtLastSuspend  = 0;
            g_pThreadMonitor->m_clocksWhileSuspended = 0;
            g_pThreadMonitor->m_sectionStackTop      = 0;
            g_pThreadMonitor->Resume(); // Trick to get current clocks into m_clocksWhileSuspended
        }
    }


    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::DeleteThreadMonitoring
    //
    //  Synopsis:
    //
    //      Prepares the thread monitoring global variables.
    //
    //------------------------------------------------------------------------

    void CWindowsServices::DeleteThreadMonitoring()
    {
        if (g_dwThreadLocalStorageIndex != TLS_OUT_OF_INDEXES)
        {
            TlsFree(g_dwThreadLocalStorageIndex);
            g_dwThreadLocalStorageIndex = TLS_OUT_OF_INDEXES;
        }
    }

    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::ReleaseMonitorBuffer
    //
    //  Synopsis:
    //
    //      Unmaps and closes any connection to xcpmon.
    //
    //------------------------------------------------------------------------

    HRESULT CWindowsServices::ReleaseMonitorBuffer(_In_ MonitorBuffer *pMonitorBuffer)
    {
        if (pMonitorBuffer != NULL)
        {
            HANDLE hMapFile = (HANDLE)(INT_PTR)pMonitorBuffer->m_fileHandle;

            UnmapViewOfFile(pMonitorBuffer);
            CloseHandle(hMapFile);

            pMonitorBuffer = NULL;
        }

        RRETURN(S_OK); // Always succeed
    }


    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::StringCchVPrintf
    //
    //  Synopsis:
    //
    //      Varargs printf support.
    //
    //------------------------------------------------------------------------

    HRESULT CWindowsServices::StringCchVPrintf(
        WCHAR         *pString,
        XUINT32        cString,
        const WCHAR   *pFormat,
        char          *pVArgs
    )
    {
        return ::StringCchVPrintf(pString, cString, pFormat, pVArgs);
    }




    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::StringCchVPrintfA
    //
    //  Synopsis:
    //
    //      Varargs printf support.
    //
    //------------------------------------------------------------------------

    HRESULT CWindowsServices::StringCchVPrintfA(
        char          *pString,
        XUINT32        cString,
        const char    *pFormat,
        char          *pVArgs
    )
    {
        return ::StringCchVPrintfA(pString, cString, pFormat, pVArgs);
    }

    //------------------------------------------------------------------------
    //
    //  Method:  CWindowsServices::YesNoMessageBox
    //
    //  Synopsis:
    //
    //      Used during debugging to report a fault such as a leak and
    //  ask whether to break into the debugger.
    //
    //------------------------------------------------------------------------

    XINT32 CWindowsServices::YesNoMessageBox(WCHAR *pContent, const WCHAR *pTitle)
    {
        UINT uType = MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST;
        IPlatformUtilities *pUtil = NULL;

        if (!FAILED(gps->GetPlatformUtilities(&pUtil)) && pUtil)
        {
            if (pUtil->IsUILanguageRTL())
            {
                uType |= (MB_RTLREADING | MB_RIGHT);
            }
        }
        return IDYES == MessageBox(
            NULL,
            pContent,
            pTitle,
            uType
        );
    }

    bool CWindowsServices::XcpVTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        void     *pVArgs          // Var args
    )
    {
        return ::XcpVTrace(uClass,pFilename,iLine,iValue,pTestString,pMessageString,pVArgs);
    }

    bool CWindowsServices::XcpTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        ...                       // Var args
    )
    {
        va_list  pVArgs;
        va_start(pVArgs, pMessageString);
        return ::XcpVTrace(uClass,pFilename,iLine,iValue,pTestString,pMessageString,pVArgs);
    }

    void CWindowsServices::XcpEnterSection(XUINT16 id)
    {
        ::XcpEnterSection(id);
    }

    void CWindowsServices::XcpLeaveSection(XUINT16 id)
    {
        ::XcpLeaveSection(id);
    }

    void CWindowsServices::XcpPopToMark()
    {
        ::XcpPopToMark();
    }

    // By default, don't enable TraceLeakTopLevel on Jupiter until the expected leaks are resolved; otherwise
    // the time to calculate causes too long of a delay and downstream problems.  This doesn't mean that the
    // checking is disabled, only the extra debug help.  To re-enambe, use xcpmon.

    XUINT32 g_DefaultTraceFlags = TraceAlways | TraceIfc | TraceAllocationStack;

    HRESULT CWindowsServices::XcpTraceMonitorInitialize(
#if DBG
        XUINT8 testMode
#endif // #if DBG
    )
    {
        m_pTraceFlags = &g_DefaultTraceFlags;
#if DBG
        if(testMode)
            return S_OK;
        else
#endif // #if DBG
            return ::XcpTraceMonitorInitialize();

    }

    void CWindowsServices::XcpTraceMonitorShutdown()
    {
        m_pTraceFlags = &g_DefaultTraceFlags;
        ::XcpTraceMonitorShutdown();
    }

    XINT32 CWindowsServices::XcpTraceMonitorAttached()
    {
        return m_pTraceFlags != &g_DefaultTraceFlags;
    }

#endif // #if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Method:   QueryPerformanceFrequency
//
//  Synopsis:
//
//------------------------------------------------------------------------
XUINT32 /* bool */
    CWindowsServices::PerformanceFrequency(
        XINT64_LARGE_INTEGER *lpFrequency)
{
    return(QueryPerformanceFrequency((LARGE_INTEGER*)(lpFrequency)));
}

//------------------------------------------------------------------------
//
//  Method:   QueryPerformanceFrequency
//
//  Synopsis:
//
//------------------------------------------------------------------------
XUINT32 /* bool */
    CWindowsServices::PerformanceCounter(
        XINT64_LARGE_INTEGER *lpPerformanceCount)
{
    return(QueryPerformanceCounter((LARGE_INTEGER*)(lpPerformanceCount)));
}

_Check_return_ HRESULT
CWindowsServices::CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Outptr_ IPALMemory** ppPALMemory)
{
    HRESULT hr = S_OK;
    CBufferMemory* pMemory = NULL;

    IFCPTR(pBuffer);
    IFCPTR(ppPALMemory);

    pMemory = new CBufferMemory(pBuffer, cbBuffer, fOwnsBuffer);

    *ppPALMemory = pMemory;
    pMemory = NULL;

Cleanup:
    ReleaseInterface(pMemory);
    RRETURN(hr);
}

#if XCP_MONITOR
//------------------------------------------------------------------------------
//
//  Method:  GetTrackerStressFromEnvironment
//
//  Check for an environment variable that enables MPStress (to test managed/native lifetime
//  management).
//
//  Values:
//      0 or any non-number:  Diabled
//      One digit number:  start iteration is 0, max iterations is the number.
//      Two digit number:  start iteration is the high order digit, max iterations is the low order digit.
//
//------------------------------------------------------------------------------

void
CWindowsServices::GetTrackerStressFromEnvironment( _Out_ int *maxIterations, _Out_ int *startIteration, _Out_ bool *backgroundGC )
{
    *maxIterations = 0;
    *startIteration = 0;
    *backgroundGC = false;

    // Check to see if the environment variable exists.

    wchar_t environmentValue[10];


    if (GetEnvironmentVariable(
            L"MPStress",
            environmentValue,
            ARRAY_SIZE(environmentValue)))
    {
        // Parse the value to an int, if possible, indicating how often stress should trigger GC
        *maxIterations = wcstoul(environmentValue, nullptr, 10 /*base*/);

        if( *maxIterations == 0 )
        {
            // Not a number.  See if it's "BG" for BackgroundGC Thread
            if (wcscmp(environmentValue, L"BG") == 0)
            {
               *backgroundGC = true;
            }

            // Otherwise assume 1
            else
            {
                *maxIterations = 1;
            }
        }

        // 2 digit numbers parse to delay/startDelay
        else if( *maxIterations >= 10 && *maxIterations < 100)
        {
            *startIteration = *maxIterations / 10;
            *maxIterations = *maxIterations %= 10;

            // Max must be >= 1
            *maxIterations = MAX( *maxIterations, 1 );

            // Start should be < max
            *startIteration = MIN( *startIteration, *maxIterations-1 );
        }

        // Everything else has no start delay
        else
        {
            *maxIterations %= 10;
        }

    }


}

#endif

