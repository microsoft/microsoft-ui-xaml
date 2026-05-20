// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#pragma warning(push)

// Ignore unreferenced parameter warnings.
#pragma warning(disable: 4100)

#include <palcore.h>
#include <verify.h>

class DebuggingServices : public IPALDebuggingServices
{
public:
    virtual XUINT32 GetTraceFlags()
    {
#if XCP_MONITOR
        // We want to log failed IFCs.
        return TraceIfc;
#else
        return 0;
#endif
    }

#if XCP_MONITOR
    virtual bool XcpVTrace(XUINT32 uClass, const XCHAR *pFilename, XINT32 iLine, XINT32 iValue, const XCHAR *pTestString, const XCHAR *pMessageString, void *pVArgs)
    {
        return true;
    }

    virtual bool XcpTrace(XUINT32 uClass, const XCHAR *pFilename, XINT32 iLine, XINT32 iValue, const XCHAR *pTestString, const XCHAR *pMessageString, ...)
    {
        return true;
    }


    virtual _Check_return_ IThreadMonitor *GetThreadMonitor()
    {
        return nullptr;
    }

    virtual HRESULT GetTraceMessageSink(IXcpTraceMessageSink** ppTraceSink)
    {
        *ppTraceSink = nullptr;
        return S_OK;
    }
#endif

    virtual void DebugOutputSz(_In_z_ const XCHAR *pString)
    {
        OutputDebugString(pString);
    }

    virtual void GetTrackerStressFromEnvironment( _Out_ int *maxIterations, _Out_ int *startIteration, _Out_ bool *backgroundGC )
    {
        *maxIterations = 0;
        *startIteration = 0;
        *backgroundGC = false;
        return;
    }

    virtual HRESULT StringCchVPrintf(WCHAR *pString, XUINT32 cString, const WCHAR *pFormat, char *pVArgs)
    {
        // Needed for XcpVTrace.
        return ::StringCchVPrintf(pString, cString, pFormat, pVArgs);
    }

    virtual HRESULT StringCchVPrintfA(char *pString, XUINT32 cString, const char *pFormat, char *pVArgs)
    {
        // Needed for XcpVTrace.
        return ::StringCchVPrintfA(pString, cString, pFormat, pVArgs);
    }

    virtual void YieldSlice()
    {
        Sleep(1);
    }

    virtual void XcpEnterSection(XUINT16 id)
    {
    }

    virtual void XcpLeaveSection(XUINT16 id)
    {
    }

    virtual void XcpPopToMark()
    {
    }

    //
    // Rest not implemented.
    //

#if XCP_MONITOR
    virtual _Check_return_ HRESULT GetMonitorBuffer(_Outptr_ MonitorBuffer **ppMonitorMemory) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT ReleaseMonitorBuffer(_In_ MonitorBuffer *pMonitorBuffer) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual void InitializeThreadMonitoring() { VERIFY_FAIL(L"not implemented"); return; } 
    virtual void DeleteThreadMonitoring() { VERIFY_FAIL(L"not implemented"); return; }
    virtual XINT32 YesNoMessageBox(XCHAR *pContent, const XCHAR *pTitle) { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual HRESULT XcpTraceMonitorInitialize(XUINT8 testMode) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT XcpTraceMonitorInitialize() { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual void  XcpTraceMonitorShutdown() { VERIFY_FAIL(L"not implemented"); return; }
    virtual XINT32  XcpTraceMonitorAttached() { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual _Check_return_ HRESULT CaptureStack(_In_ XUINT32 cMaxCallers, _Out_ XUINT32   *pcCallers, _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers, _In_ XUINT32 cIgnoreLevels) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT GetCallerSourceLocations(_In_ XUINT32 cCallers, _In_reads_(cCallers) XUINT64 *pCallers, _Outptr_result_buffer_(cCallers) ICallingMethod **ppCallingMethods) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual void FreeCallingMethods(ICallingMethod *pCallers) { VERIFY_FAIL(L"not implemented"); return; }
    virtual void SetTraceMessageSink(IXcpTraceMessageSink* pTraceSink) { VERIFY_FAIL(L"not implemented"); return; }
#endif
    virtual _Check_return_ HRESULT XcpPerfMarker(_In_ XUINT64 lMarker) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, XCHAR* szMarkerInfo) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, XDWORD nMarkerInfo) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual PerfMarkerType PerfMarkersEnabled() { VERIFY_FAIL(L"not implemented"); return PerfMarkersDisabled; }
    virtual void InitDebugTrace() { VERIFY_FAIL(L"not implemented"); }
    virtual void CleanupDebugTrace() { VERIFY_FAIL(L"not implemented"); }
    virtual bool IsDebugTraceTypeActive(_In_ XDebugTraceType traceType) { VERIFY_FAIL(L"not implemented"); return FALSE; }
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const XCHAR *pFormattedMsg, ...) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const XCHAR *pFormattedMsg, _In_ va_list args) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT DebugOutputSzNoEndl(_In_z_ const XCHAR *pFormattedMsg, ...) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ bool  DebugAssertMessageBox(_In_z_ const XCHAR *pCondition, _In_z_ const XCHAR *pFile, XUINT32 nLine) { VERIFY_FAIL(L"not implemented"); return FALSE; }
    virtual void  DebugBreak() { VERIFY_FAIL(L"not implemented"); return; }
    virtual bool TESTSignalMemLeak() { VERIFY_FAIL(L"not implemented"); return false; }
    virtual void TESTSignalManagedMemLeak(_Out_ bool *showAssert) { VERIFY_FAIL(L"not implemented"); *showAssert = FALSE; }
    virtual void TESTSignalASSERT(_In_z_ const XCHAR *pString) { VERIFY_FAIL(L"not implemented"); }
    virtual void TESTSignalReferenceTrackerLeak() { VERIFY_FAIL(L"not implemented"); }
    virtual bool IsStressModeEnabled() { return false; }
    virtual XUINT32 GetStressLeakThreshold() { return 0; }
    virtual void RecordLock(void *) { VERIFY_FAIL(L"not implemented"); return; }
    virtual void RemoveLock(void *) { VERIFY_FAIL(L"not implemented"); return; }
    virtual void CheckLocks() { VERIFY_FAIL(L"not implemented"); return; }
    virtual HRESULT XcpRenderCountersInitialize() { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT XcpRenderCountersIncrement(_In_ XUINT32 uCounter) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual void SetTraceFlags(XUINT32* pFlags) { VERIFY_FAIL(L"not implemented"); return; }
    virtual XUINT64 GetMemoryCount() { VERIFY_FAIL(L"not implemented"); return 0; };
};

class MemoryServices : public IPALMemoryServices
{

public:
    MemoryServices()
    {
    }

    //
    // Rest not implemented.
    //

    virtual _Check_return_ HRESULT CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Outptr_ IPALMemory** ppPALMemory) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Outptr_ IPALStream **ppPALStream) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
};

class ThreadingServices : public IPALThreadingServices
{
    // Rest not implemented.

    virtual HRESULT ThreadCreate(_Outptr_ IPALWaitable **ppThread, _In_ PALTHREADPFN pfn, _In_ XUINT32 cData, _In_reads_(cData) XUINT8 *pData) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT EventCreate(_Outptr_ IPALEvent **ppEvent, _In_ XINT32 bSignaled, _In_ XINT32 bManual) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT QueueCreate(_Outptr_ IPALQueue **ppQueue) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT NamedEventCreate(_Outptr_ IPALEvent** ppEvent, PAL_IPCInitMode initMode, bool bInitialState, bool bManualReset, _In_ const xstring_ptr_view& strName, _In_ bool) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual XUINT32 WaitForObjects(_In_ XUINT32 cWaitable, _In_ IPALWaitable **ppWaitable, _In_ XINT32 bAll, _In_ XUINT32 nTimeout) { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual XINT32  ThreadGetPriority(_In_opt_ IPALWaitable *pThread) { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual HRESULT ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual XINT32  ProcessGetPriority() { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual HRESULT ProcessSetPriority(_In_ XINT32 nPriority) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
};

class CoreServices : public IPALCoreServices
{
public:
    CoreServices()
    {}

    // File services

    virtual _Check_return_ HRESULT FileCreate(_In_ XUINT32 cFileName, _In_reads_(cFileName) const XCHAR *pFileName, XINT32 fileOptions, _Outptr_ IPALFile **ppFile)
    {
        return E_NOTIMPL;
    }

    // OS System Settings

    virtual _Check_return_ HRESULT GetPlatformUtilities(_Outptr_ IPlatformUtilities** ppInterface)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT GetSystemColor(_In_ XINT32 colorIndex, _Out_ XINT32 *pSysColor)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    // Time services

    virtual               XUINT64 GetCPUTime()
    {
        ASSERT(FALSE);
        return 0;
    }

    virtual               XUINT64 GetCPUMilliseconds()
    {
        ASSERT(FALSE);
        return 0;
    }

    virtual _Check_return_ HRESULT CreateClock(_Outptr_ IPALClock **ppIClock)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    // CPU Info
    virtual _Check_return_ HRESULT GetNumberOfCPUCores(_Out_ XUINT32 *pcProcessorCores)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    virtual _Check_return_ bool IsSupportedPlatform()
    {
        ASSERT(FALSE);
        return false;
    }

    // Process wide control mechanisms
    virtual _Check_return_ HRESULT GetApplicationSingleton(_Outptr_ IPALApplicationSingleton **ppSingleton)
    {
        *ppSingleton = nullptr;
        RRETURN(S_OK);
    }

    virtual XUINT32 GenerateSecurityToken()
    {
        ASSERT(FALSE);
        return 0;
    }

    // Keyboard states
    virtual _Check_return_ HRESULT GetKeyboardModifiersState(_Out_ XUINT32 *pModifiers)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    virtual XUINT32 /* bool */ PerformanceFrequency(_Out_ XINT64_LARGE_INTEGER *lpFrequency)
    {
        ASSERT(FALSE);
        return 0;
    }

    virtual XUINT32 /* bool */ PerformanceCounter(_Out_ XINT64_LARGE_INTEGER *lpPerformanceCount)
    {
        ASSERT(FALSE);
        return 0;
    }

    // COM initialization

    virtual _Check_return_ HRESULT CallCoInitializeMTA()
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }

    virtual void CallCoUninitialize()
    {
        ASSERT(FALSE);
    }

    // Remote desktop
    virtual _Check_return_ HRESULT IsOnRemoteDesktopSession(_Out_ bool* pfOnRemoteDesktop)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }
};

IPALDebuggingServices* GetPALDebuggingServices()
{
    static DebuggingServices gds;
    return &gds;
}

IPALMemoryServices* GetPALMemoryServices()
{
    static MemoryServices gms;
    return &gms;
}

IPALThreadingServices* GetPALThreadingServices()
{
    static ThreadingServices gts;
    return &gts;
}

IPALCoreServices* GetPALCoreServices()
{
    static CoreServices coreServices;
    return &coreServices;
}

#pragma warning(pop)
