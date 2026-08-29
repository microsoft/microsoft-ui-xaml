// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the basic debugging support for the core platform
//      abstraction layer

#ifndef __PAL__DEBUGGING__
#define __PAL__DEBUGGING__

#include <vadefs.h>

#include "xcpdebug.h"        // Required defines/enums for debugging
#include "minerror.h"

struct IPALSurface;

#define RENDERCOUNTERS DBG

//------------------------------------------------------------------------
//
//  Section:  Development support macros
//
//  Synopsis:
//
//      Provides macros and functions for tracing, assertion and performance
//      measurement.
//
//      Functionality is provided only in debug builds.
//
//------------------------------------------------------------------------

// A STATIC_ASSERT is checked at compile time and has NO runtime impact.
// (Note: the [1] causes MSVC to display the type name which is kind of an error message.
#define STATIC_ASSERT(expression, msg)  typedef char Static_Assert_##msg[1][(expression) ? 1 : 0]

#if DBG && XCP_MONITOR

    #define RAWTRACE(flag, ...) \
        do { \
            if(GetPALDebuggingServices()->GetTraceFlags() & flag) GetPALDebuggingServices()->XcpTrace(MonitorRaw, __WFILE__,__LINE__, 0, NULL,WIDEN_VA(,#__VA_ARGS__),##__VA_ARGS__); \
        } while(0)

    #define Trace(x) TRACE(TraceAlways, (x))

#else // #if DBG && XCP_MONITOR

    #define RAWTRACE(flag, ...)
    #define Trace(x)

#endif // #else #if DBG && XCP_MONITOR


#if XCP_MONITOR
    #define ENTERSECTION(id) {GetPALDebuggingServices()->XcpEnterSection(Section##id);       if(GetPALDebuggingServices()->GetTraceFlags() & TraceSection){GetPALDebuggingServices()->XcpTrace(MonitorEnterSection, __WFILE__,__LINE__,Section##id,XCPW(#id),NULL);}}
    #define LEAVESECTION(id) {GetPALDebuggingServices()->XcpLeaveSection(Section##id);       if(GetPALDebuggingServices()->GetTraceFlags() & TraceSection){GetPALDebuggingServices()->XcpTrace(MonitorLeaveSection, __WFILE__,__LINE__,Section##id,XCPW(#id),NULL);}}
    #define ENTERMETHOD      {GetPALDebuggingServices()->XcpEnterSection(METHOD_ENTRY_MARK); if(GetPALDebuggingServices()->GetTraceFlags() & TraceSection){GetPALDebuggingServices()->XcpTrace(MonitorEnterMethod,  __WFILE__,__LINE__,0, NULL,NULL);}}
    #define LEAVEMETHOD      {GetPALDebuggingServices()->XcpPopToMark();                     if(GetPALDebuggingServices()->GetTraceFlags() & TraceSection){GetPALDebuggingServices()->XcpTrace(MonitorLeaveMethod,  __WFILE__,__LINE__,0, NULL,NULL);}}
#else
    #define ENTERSECTION(id)
    #define LEAVESECTION(id)
    #define ENTERMETHOD
    #define LEAVEMETHOD
#endif


// IFCEXPECT_ASSERT behaviour gives ASSERT behaviour in debug builds, IFXEXPECT behaviour in free builds.
//#define IFCEXPECT_ASSERT(cond) {_Analysis_assume_(cond); ASSERT(cond); IFCEXPECT(cond);}
#define IFCEXPECT_ASSERT(...) {ASSERT(__VA_ARGS__); IFCEXPECT(__VA_ARGS__);}
#define IFCEXPECT_ASSERT_RETURN(...) {ASSERT(__VA_ARGS__); IFCEXPECT_RETURN(__VA_ARGS__);}
#define IFCEXPECT_ASSERT_NOTRACE(...) {ASSERT(__VA_ARGS__); IFCEXPECT_NOTRACE(__VA_ARGS__);}
#define IFCEXPECT_ASSERT_NOTRACE_RETURN(...) {ASSERT(__VA_ARGS__); IFCEXPECT_NOTRACE_RETURN(__VA_ARGS__);}

#define IfNullAssertRet(expr) { ASSERT(expr); if ((expr) == NULL) { return E_INVALIDARG; } }
#define IfNullAssertGo(expr) if ((expr) == NULL) { ASSERT(expr); hr = E_UNEXPECTED; goto OnError; }
#define IfFailAssertGo(hr) { ASSERT(SUCCEEDED(hr), "SUCCEEDED(" #hr ")"); if (FAILED(hr)) { goto OnError; } }
#define IfFailAssert(hr) { ASSERT(SUCCEEDED(hr), "SUCCEEDED(" #hr ")"); }
#define IfFalseAssertGo(expr) if (!(expr)) { ASSERT(expr); hr = E_UNEXPECTED; goto OnError; }
#define IfFalseAssertRet(expr) if (!(expr)) { ASSERT(expr); return E_UNEXPECTED; }
#define IfFalseGo(expr) if (!(expr)) { hr = E_UNEXPECTED; goto OnError; }
#define IfFailGo(hr) if (FAILED(hr)) { goto OnError; }
#define IfNullGo(expr) if ((expr) == NULL) { hr = E_UNEXPECTED; goto OnError; }

//------------------------------------------------------------------------
//
//  Enum:  XDebugTraceType
//
//  Synopsis:
//      Enum describing the tasks requested to the DebugTrace method.
//
//------------------------------------------------------------------------
enum XDebugTraceType
{
    // Flags specific to Output debug strings performed with OutputDebugString.
    XCP_TRACE_OUTPUT_MSG = 0x00000001,
    XCP_TRACE_PREPEND_OUTPUT_WITH_THREAD_ID = 0x00000002,
    XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID = 0x00000004,
    XCP_TRACE_NO_NEW_LINE_IN_OUTPUT_MSG = 0x00000008,

    // Flags defining the ETW trace levels
    XCP_TRACE_ERROR = 0x00000010,
    XCP_TRACE_WARNING = 0x00000020,
    XCP_TRACE_VERBOSE = 0x00000040,

    // Values defining the ETW trace categories
    XCP_TRACE_COMMON = 0x00010000,
    XCP_TRACE_DM_COMPOSITOR = 0x00020000,
    XCP_TRACE_DM_INPUT_MANAGER = 0x00030000,
    XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT = 0x00040000,
    XCP_TRACE_DM_PAL_SERVICE = 0x00050000,
    XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER = 0x00060000,
    XCP_TRACE_DM_SCROLLVIEWER = 0x00070000,
    XCP_TRACE_DM_SCROLLCONTENTPRESENTER = 0x00080000,
    XCP_TRACE_RESOURCELOADING = 0x00090000,
    XCP_TRACE_LISTVIEWBASEITEMCHROME = 0x000A0000,
    XCP_TRACE_CALENDARVIEWBASEITEMCHROME = 0x000B0000
    // Add tracing categories here 0x000C0000, 0x000D0000, etc...
    // and update CWindowsServices::IsDebugTraceTypeActive,
    // CWindowsServices::DebugTrace and xcp\win\inc\WPPTracing.h
};

// Bit mask used to specify which WPP flag to use for a debugging ETW trace.
#define XCP_TRACE_WPP_FLAGS_MASK 0xFFFF0000

//------------------------------------------------------------------------
//
//  Enum:  XTraceType
//
//  Synopsis:
//      Enum describing the types of Perf Trace class types that can
//       be used for event tracing.
//
//------------------------------------------------------------------------
enum XTraceType
{
    XCP_TRACE_START = 0,
    XCP_TRACE_END = 1,
    XCP_TRACE_INFORMATION = 2
};

//------------------------------------------------------------------------
//
//  Enum:  PerfMarkerType
//
//  Synopsis:
//      Enum describing the types of Perf markers enabled.
//      This allows us to distinguish between the markers and allow tracing to be enabled
//      when PerfEnabledMarker is not enabled but the BrowserControlEnabledMarker
//      is enabled.
//
//------------------------------------------------------------------------
enum PerfMarkerType
{
    PerfMarkersDisabled = 0,
    PerfMarkerEnabled = 1,
    PerfBrowserControlMarkerEnabled = 2,
    PerfBothMarkersEnabled = 3
};


//------------------------------------------------------------------------
//
//  PAL Tracing Interfaces
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Interface:  IXcpRenderCounters
//
//  Synopsis:
//      An object to hold counters related to rendering.
//------------------------------------------------------------------------

class IXcpRenderCounters
{
public:
    virtual HRESULT Increment(_In_ XUINT32 uCounter) = 0;
    virtual ~IXcpRenderCounters() { }
};

enum XcpRenderCounters
{
    XcpRenderCountersGenerateEdges
};

#if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Interface:  IXcpTraceMessageSink
//
//  Synopsis:
//      Support for being able to proxy xcpmonitor's traces to another location.
//      In this case they are redirected to the test host
//------------------------------------------------------------------------
struct IXcpTraceMessageSink
{
    virtual void AcceptTraceMessage(
                       XUINT32  uClass,      // Class of monitoring event, a.g. assertion, trace
            _In_opt_z_ WCHAR   *pFileName,
                       XINT32   iLine,
                       XINT32   iValue,      // E.g. assertion value, HRESULT
            _In_opt_z_ WCHAR   *pTestString, // E.g. Assertion string
            _In_opt_z_ WCHAR   *pMessage    // Message text
            ) = 0;
};


//------------------------------------------------------------------------
//
//  Interface:  IThreadMonitor
//
//  Synopsis:
//      Per-thread debug monitoring data
//
//------------------------------------------------------------------------


#define METHOD_ENTRY_MARK 0xFFFF

struct IThreadMonitor
{
protected:
    IThreadMonitor()
    {
        m_sectionStack[0] = SectionMain; // All threads start in the 'main' section
        m_sectionStackTop = 1;
    }
    ~IThreadMonitor(){}
public:
    XUINT64  m_clocksAtLastSuspend;
    XUINT64  m_clocksWhileSuspended;
    XUINT16  m_sectionStack[512];
    XUINT16  m_sectionStackTop;
    XUINT32  m_threadId;
    virtual void Suspend() = 0;
    virtual void Resume() = 0;
};

struct ICallingMethod
{
    WCHAR   szFilename[64];
    WCHAR   szSymbolName[256];
    XUINT32 iLine;
};

#endif // #if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Interface:  IPALDebuggingServices
//
//  Synopsis:
//      Provides an abstraction for system debugging support.
//
//------------------------------------------------------------------------

struct MonitorBuffer;

struct IPALDebuggingServices
{
#if XCP_MONITOR
    // Debugging support - services provided to Jolt components

    virtual bool XcpVTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        void     *pVArgs          // Var args
    ) = 0;

    virtual bool XcpTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        ...                       // Var args
    ) = 0;

#endif // end #if XCP_MONITOR

#if XCP_MONITOR

    virtual               void            YieldSlice() = 0;  // Sleep a short while
    virtual _Check_return_ HRESULT         GetMonitorBuffer(_Out_ MonitorBuffer **ppMonitorMemory) = 0;
    virtual _Check_return_ IThreadMonitor *GetThreadMonitor() = 0;
    virtual _Check_return_ HRESULT         ReleaseMonitorBuffer(_In_ MonitorBuffer *pMonitorBuffer) = 0;
    virtual               void            InitializeThreadMonitoring() = 0;
    virtual               void            DeleteThreadMonitoring() = 0;
    virtual               HRESULT         StringCchVPrintf(WCHAR *pString, XUINT32 cString, const WCHAR *pFormat, char *pVArgs) = 0;
    virtual               HRESULT         StringCchVPrintfA(char *pString, XUINT32 cString, const char *pFormat, char *pVArgs) = 0;
    virtual               XINT32          YesNoMessageBox(WCHAR *pContent, const WCHAR *pTitle) = 0;


    virtual void XcpEnterSection(XUINT16 id) = 0;
    virtual void XcpLeaveSection(XUINT16 id) = 0;
    virtual void XcpPopToMark() = 0;

#if DBG
    virtual HRESULT XcpTraceMonitorInitialize(XUINT8 testMode) = 0;
#else
    virtual HRESULT XcpTraceMonitorInitialize() = 0;
#endif // #if DBG

    virtual void    XcpTraceMonitorShutdown() = 0;
    virtual XINT32  XcpTraceMonitorAttached() = 0;

    // Stack trace support

    virtual _Check_return_ HRESULT CaptureStack(
        _In_                            XUINT32    cMaxCallers,
        _Out_                           XUINT32   *pcCallers,
        _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers,
        _In_                            XUINT32    cIgnoreLevels
    ) = 0;

    virtual _Check_return_ HRESULT GetCallerSourceLocations(
        _In_                         XUINT32          cCallers,
        _In_reads_(cCallers)        XUINT64         *pCallers,
        _Outptr_result_buffer_(cCallers) ICallingMethod **ppCallingMethods  // Receives pointer to array of cCallers ICallingMethods
    ) = 0;

    virtual void FreeCallingMethods(ICallingMethod *pCallers) = 0;
#if DBG

#pragma region // Additional methods to support setting a proxy target that also gets the trace messages
    virtual void SetTraceMessageSink(IXcpTraceMessageSink* pTraceSink) = 0;
    virtual HRESULT GetTraceMessageSink(IXcpTraceMessageSink** ppTraceSink) = 0;
#pragma endregion

#endif //DBG

#endif // #if XCP_MONITOR

// Performance measurement

    virtual _Check_return_ HRESULT XcpPerfMarker(_In_ XUINT64 lMarker) = 0;
    virtual _Check_return_ HRESULT XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, _In_z_ WCHAR* szMarkerInfo) = 0;
    virtual _Check_return_ HRESULT XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, XDWORD nMarkerInfo) = 0;
    virtual PerfMarkerType PerfMarkersEnabled() = 0;

    virtual _Check_return_ HRESULT XcpRenderCountersInitialize() = 0;
    virtual _Check_return_ HRESULT XcpRenderCountersIncrement(_In_ XUINT32 uCounter) = 0;

    // Debugging support - services used by the debugging support code
    virtual               void    InitDebugTrace() = 0;
    virtual               void    CleanupDebugTrace() = 0;
    virtual               bool   IsDebugTraceTypeActive(_In_ XDebugTraceType traceType) = 0;
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, ...) = 0;
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, _In_ va_list args) = 0;
    virtual _Check_return_ HRESULT DebugOutputSzNoEndl(_In_z_ const WCHAR *pFormattedMsg, ...) = 0;
    virtual               void    DebugOutputSz(_In_z_ const WCHAR *pString) = 0;

#if DBG

    virtual _Check_return_ bool  DebugAssertMessageBox(
                                   _In_z_ const WCHAR *pCondition,
                                   _In_z_ const WCHAR *pFile,
                                   XUINT32 nLine) = 0;

    virtual bool TESTSignalMemLeak() = 0;

    virtual void TESTSignalManagedMemLeak(_Out_ bool * showAssert) = 0;

    virtual void TESTSignalASSERT(_In_z_ const WCHAR *pString) = 0;

    virtual void TESTSignalReferenceTrackerLeak() = 0;

    virtual bool IsStressModeEnabled() = 0;

    virtual XUINT32 GetStressLeakThreshold() = 0;

#endif // #if DBG

// trace flag settings
    virtual void SetTraceFlags(XUINT32* pFlags) = 0;
    virtual XUINT32 GetTraceFlags() = 0;
    virtual XUINT64 GetMemoryCount() = 0;

    // Enable MPStress with an environment variable
#if XCP_MONITOR
    virtual void GetTrackerStressFromEnvironment( _Out_ int *maxIterations, _Out_ int *startIteration, _Out_ bool *backgroundGC ) = 0;
#endif
};

#endif //#ifndef __PAL__DEBUGGING__
