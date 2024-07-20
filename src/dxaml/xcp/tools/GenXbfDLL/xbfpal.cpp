// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <cassert>
#include <ReaderString.h>
#include <WinStream.h>
#include "XcpAllocation.h"

EncodedPtr<IPlatformServices> gps; // will stay null

#define VERIFY_FAIL(x) ::DebugBreak()

#if XCP_MONITOR
class DummyThreadMonitor : public IThreadMonitor
{
public:
    DummyThreadMonitor() { m_clocksWhileSuspended = 0; m_threadId = 0; }
    ~DummyThreadMonitor() {}
    virtual void Suspend() {}
    virtual void Resume() {}
};
#endif

class DebuggingServices: public IPALDebuggingServices
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
        return ::XcpVTrace(uClass, pFilename, iLine, iValue, pTestString, pMessageString, pVArgs);
    }

    virtual bool XcpTrace(XUINT32 uClass, const XCHAR *pFilename, XINT32 iLine, XINT32 iValue, const XCHAR *pTestString, const XCHAR *pMessageString, ...)
    {
        va_list vargs;
        va_start(vargs, pMessageString);
        return XcpVTrace(uClass, pFilename, iLine, iValue, pTestString, pMessageString, vargs);
    }


    virtual _Check_return_ IThreadMonitor *GetThreadMonitor()
    {
        // Needed for XcpVTrace.
        static DummyThreadMonitor threadMonitor;
        return &threadMonitor;
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

#if XCP_MONITOR
    virtual HRESULT StringCchVPrintf(XCHAR *pString, XUINT32 cString, const XCHAR *pFormat, char *pVArgs)
    {
        // Needed for XcpVTrace.
        return ::StringCchVPrintf(pString, cString, pFormat, pVArgs);
    }

    virtual HRESULT StringCchVPrintfA(char *pString, XUINT32 cString, const char *pFormat, char *pVArgs)
    {
        // Needed for XcpVTrace.
        return ::StringCchVPrintfA(pString, cString, pFormat, pVArgs);
    }

#endif
    
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
    virtual _Check_return_ HRESULT GetMonitorBuffer(_Out_ MonitorBuffer **ppMonitorMemory) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
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
    virtual void SetTraceMessageSink(IXcpTraceMessageSink* pTraceSink) { VERIFY_FAIL(L"not implemented"); return ; }
#endif
    virtual _Check_return_ HRESULT XcpPerfMarker(_In_ XUINT64 lMarker) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, XCHAR* szMarkerInfo) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, XDWORD nMarkerInfo) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual PerfMarkerType PerfMarkersEnabled() { VERIFY_FAIL(L"not implemented"); return PerfMarkersDisabled; }
    virtual void InitDebugTrace() { VERIFY_FAIL(L"not implemented"); }
    virtual void CleanupDebugTrace() { VERIFY_FAIL(L"not implemented"); }
    virtual bool IsDebugTraceTypeActive(_In_ XDebugTraceType traceType) { VERIFY_FAIL(L"not implemented"); return false; }
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const XCHAR *pFormattedMsg, ...) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const XCHAR *pFormattedMsg, _In_ va_list args) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT DebugOutputSzNoEndl(_In_z_ const XCHAR *pFormattedMsg, ...) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ bool  DebugAssertMessageBox( _In_z_ const XCHAR *pCondition, _In_z_ const XCHAR *pFile, XUINT32 nLine) { VERIFY_FAIL(L"not implemented"); return false; }
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
private:

public:
    MemoryServices()
    {
    }

    virtual _Check_return_ HRESULT CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Out_ IPALMemory** ppPALMemory) 
    {         
        xref_ptr<CBufferMemory> pMemory;

        IFCPTR_RETURN(pBuffer);
        IFCPTR_RETURN(ppPALMemory);

        pMemory = make_xref<CBufferMemory>(pBuffer, cbBuffer, fOwnsBuffer);

        *ppPALMemory = pMemory.detach();

        return S_OK;
    }

    virtual _Check_return_ HRESULT CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Out_ IPALStream **ppPALStream)
    {
        xref_ptr<CWinDataStream> pStream;
        xref_ptr<CWinDataStreamBuffer> pWinDataStreamBuffer;

        IFCPTR_RETURN(pPALMemory);
        IFCPTR_RETURN(ppPALStream);

        IFC_RETURN(CWinDataStreamBuffer::CreateFromIPalMemory(pPALMemory, pWinDataStreamBuffer.ReleaseAndGetAddressOf()));
        IFC_RETURN(CWinDataStream::Create(pWinDataStreamBuffer, pStream.ReleaseAndGetAddressOf()));

        *ppPALStream = pStream.detach();

        return S_OK;
    }
};


extern _Check_return_ HRESULT NullTerminateString(_In_ XUINT32 cString, _In_reads_(cString) const XCHAR *pString, _Outptr_result_z_ wchar_t **ppwsz);

class CoreServices : public IPALCoreServices
{
public:
    CoreServices()
    {}

    // File services

    virtual _Check_return_ HRESULT FileCreate(_In_ XUINT32 cFileName, _In_reads_(cFileName) const XCHAR *pFileName, XINT32 fileOptions, _Out_ IPALFile **ppFile)
    {
        HRESULT hr;
        CWinFile *pFile = NULL;
        wchar_t *pwsz = NULL;
        DWORD dwDesiredAccess = 0;
        DWORD dwShareMode = 0;
        DWORD dwCreationDisposition = 0;

        if (fileOptions == foptReserved1 || fileOptions == foptReserved2)
        {
            // If you hit this assert, the calling code was written for an older version of the FileCreate API.
            // To fix it, update the calling code:
            //    FileCreate(cchName, wszName, TRUE, &pFile)  -> FileCreate(cchName, wszName, foptWrite, &pFile)
            //    FileCreate(cchName, wszName, FALSE, &pFile) -> FileCreate(cchName, wszName, foptRead, &pFile)
            ASSERT(FALSE);
            IFC(E_INVALIDARG);
        }

        pFile = new CWinFile;

    // On NT we need to pass a NULL terminate string to CreateFile.

        IFC(NullTerminateString(cFileName, pFileName, &pwsz));

    // Attempt to open the file

        hr = E_FAIL;

        if (fileOptions & foptRead && fileOptions & foptWrite)
        {
            // Read and write access, no sharing
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            dwShareMode = 0;
            dwCreationDisposition = OPEN_ALWAYS;
        }
        else if (fileOptions & foptRead)
        {
            // Read-only access
            dwDesiredAccess = GENERIC_READ;
            dwShareMode = FILE_SHARE_READ;
            dwCreationDisposition = OPEN_EXISTING;
        }
        else if (fileOptions & foptWrite)
        {
            // Write-only access
            dwDesiredAccess = GENERIC_WRITE;
            dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
            dwCreationDisposition = CREATE_ALWAYS;
        }
        else
        {
            // File opened without any access, fail
            IFC(E_INVALIDARG);
        }

        if (fileOptions & foptExclusive)
        {
            // Get exclusive access to the file
            dwShareMode = 0;
        }

        pFile->m_hSystem = CreateFile(pwsz,
                                      dwDesiredAccess,
                                      dwShareMode,
                                      NULL,
                                      dwCreationDisposition,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

        if ((INVALID_HANDLE_VALUE == pFile->m_hSystem) || (NULL == pFile->m_hSystem))
            goto Cleanup;

       *ppFile = static_cast<IPALFile *>(pFile);
        pFile = NULL;
        hr = S_OK;

    Cleanup:
        delete pFile;

        if (pwsz)
            delete[] pwsz;

        RRETURN(hr);
    }

// OS System Settings

    virtual _Check_return_ HRESULT GetPlatformUtilities(_Outptr_ IPlatformUtilities** ppInterface)
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
    }

    virtual _Check_return_ HRESULT GetSystemColor(_In_ XINT32 colorIndex, _Out_ XINT32 *pSysColor)
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
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

    virtual _Check_return_ HRESULT CreateClock(_Out_ IPALClock **ppIClock)
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
    }

// CPU Info
    virtual _Check_return_ HRESULT GetNumberOfCPUCores(_Out_ XUINT32 *pcProcessorCores)
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
    }

    virtual bool IsSupportedPlatform()
    {
        ASSERT(FALSE);
        return false;
    }

// Process wide control mechanisms
    virtual _Check_return_ HRESULT GetApplicationSingleton(_Out_ IPALApplicationSingleton **ppSingleton)
    {
        ASSERT(false);
        return S_OK;
    }

    virtual XUINT32 GenerateSecurityToken()
    {
        ASSERT(FALSE);
        return 0;
    }

// Keyboard states
    virtual _Check_return_ HRESULT GetKeyboardModifiersState( _Out_ XUINT32 *pModifiers)
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
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
        RRETURN(E_NOTIMPL);
    }

    virtual void CallCoUninitialize()
    {
        ASSERT(FALSE);
    }

// Remote desktop
    virtual _Check_return_ HRESULT IsOnRemoteDesktopSession( _Out_ bool* pfOnRemoteDesktop )
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
    }
};

class ThreadingServices: public IPALThreadingServices
{
    // Rest not implemented.

    virtual HRESULT ThreadCreate(_Out_ IPALWaitable **ppThread, _In_ PALTHREADPFN pfn, _In_ XUINT32 cData, _In_reads_(cData) XUINT8 *pData) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT EventCreate(_Out_ IPALEvent **ppEvent, _In_ XINT32 bSignaled, _In_ XINT32 bManual) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual HRESULT QueueCreate(_Out_ IPALQueue **ppQueue) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual _Check_return_ HRESULT NamedEventCreate(_Outptr_ IPALEvent** ppEvent, PAL_IPCInitMode initMode, bool bInitialState, bool bManualReset, _In_ const xstring_ptr_view& strName, _In_ bool) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual XUINT32 WaitForObjects(_In_ XUINT32 cWaitable, _In_ IPALWaitable **ppWaitable, _In_ XINT32 bAll, _In_ XUINT32 nTimeout) { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual XINT32  ThreadGetPriority(_In_opt_ IPALWaitable *pThread) { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual HRESULT ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
    virtual XINT32  ProcessGetPriority() { VERIFY_FAIL(L"not implemented"); return 0; }
    virtual HRESULT ProcessSetPriority(_In_ XINT32 nPriority) { VERIFY_FAIL(L"not implemented"); return E_NOTIMPL; }
};

class URIServices : public IPALURIServices
{
public:
    virtual _Check_return_ HRESULT UriCreate(
                                  _In_ XUINT32 cString,
                                  _In_reads_(cString) const XCHAR *pString,
                                  _Out_ IPALUri **ppUri)
    {
        ASSERT(FALSE);
        return E_NOTIMPL;
    }
};

IPALDebuggingServices *GetPALDebuggingServices()
{
    static DebuggingServices debuggingServices;
    return &debuggingServices;
}

IPALMemoryServices *GetPALMemoryServices()
{
    static MemoryServices memoryServices;
    return &memoryServices;
}

IPALURIServices *GetPALURIServices()
{
    static URIServices uriServices;
    return &uriServices;
}

IPALMathServices *GetPALMathServices()
{
    return nullptr;
}

IPALPrintIOServices *GetPALPrintIOServices()
{
    return nullptr;
}

IPALThreadingServices *GetPALThreadingServices()
{
    static ThreadingServices threadingServices;
    return &threadingServices;
}

IPALTextServices *GetPALTextServices()
{
    return nullptr;
}

IPALCoreServices *GetPALCoreServices()
{
    static CoreServices coreServices;
    return &coreServices;
}

_Check_return_ HRESULT
ObtainPlatformServices(
#if DBG
    _Out_ IPlatformServices **ppInterface,
    XUINT8 testMode
#else
    _Out_ IPlatformServices **ppInterface
#endif // #if DBG
)
{
    HRESULT hr = S_OK;

    //static XbfPlatformServices xbfPal;
    //*ppInterface = &xbfPal;

    RRETURN(hr);
}
