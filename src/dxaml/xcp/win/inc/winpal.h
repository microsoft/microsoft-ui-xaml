// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FeatureFlags.h>
#include "ConcurrencySal.h"
#include <vadefs.h>
#include "ReaderString.h"

class WindowsGraphicsDeviceManager;

namespace ABI::Microsoft::UI::Input {
    interface IInputKeyboardSourceStatics;
}

//------------------------------------------------------------------------
//
//  Class:  MsgPacket
//
//  Synopsis:
//      Windows specific message packet from browser
//
//------------------------------------------------------------------------
struct MsgPacket
{
    HWND m_hwnd;     // Null means windowless non null means windowed
    HWND m_hwndContainer; // Null means windowed, non null means windowless
    XINT32 m_Top;    // 0 means windowed else actual offset
    XINT32 m_Left;   // 0 means windowed else actual offset
    WPARAM m_wParam;
    LPARAM m_lParam;
    XHANDLE m_pCoreWindow;

    ixp::IPointerPoint* m_pPointerPointNoRef;
    ixp::IPointerEventArgs* m_pPointerEventArgsNoRef;
    bool m_isNonClientPointerMessage{ false };

    //Ctor
    MsgPacket()
    {
        m_hwnd = NULL;
        m_hwndContainer = NULL;
        m_Top = 0;
        m_Left = 0;
        m_wParam = NULL;
        m_lParam = NULL;
        m_pCoreWindow = NULL;

        m_pPointerPointNoRef = nullptr;
        m_pPointerEventArgsNoRef = nullptr;
    }

    // Destructor
    ~MsgPacket()
    {
        m_hwnd = NULL;
        m_hwndContainer = NULL;
        m_Top = 0;
        m_Left = 0;
        m_wParam = NULL;
        m_lParam = NULL;
        m_pCoreWindow = NULL;

        m_pPointerPointNoRef = nullptr;
        m_pPointerEventArgsNoRef = nullptr;
    }
};

//------------------------------------------------------------------------
//
//  Class:  CMappedMemory
//
//  Synopsis:
//      A memory mapped file object.
//
//------------------------------------------------------------------------

class CMappedMemory final : public IPALMemory, public CInterlockedReferenceCount
{
private:
   ~CMappedMemory() override;

public:
    CMappedMemory(
        _In_ IPALFile *pOwner,
        _In_ HANDLE hMapped,
        _In_ void *pMapped,
        _In_ XUINT32 cMapped,
        _In_ XUINT32 cOffset = 0);

// Reference count implementation

    XUINT32 AddRef()  const override {return CInterlockedReferenceCount::AddRef();}
    XUINT32 Release() const override {return CInterlockedReferenceCount::Release();}

// IPALMemory implementation

    XUINT32 GetSize()    const override  { return m_cMapped; }
    void   *GetAddress() const override  { return m_pMapped; }

// Additional Methods
    // GetOffset added to represent the offset of the mapped memory view to the
    // entire mapped file.
    XUINT32 GetOffset() const   { return m_cOffset; }

private:
    XUINT32          m_cMapped;
    IPALFile        *m_pOwner;
    HANDLE           m_hMapped;
    void            *m_pMapped;
    XUINT32          m_cOffset;
};

//------------------------------------------------------------------------
//
//  Class:  CBufferMemory
//
//  Synopsis:
//      An IPALMemory compliant wrapper for a buffer.
//
//------------------------------------------------------------------------

class CBufferMemory final : public IPALMemory, public CInterlockedReferenceCount
{
private:
   ~CBufferMemory() override;

public:
    CBufferMemory(
        _In_reads_(cBuffer) XUINT8 *pBuffer,
        _In_ XUINT32 cBuffer,
        _In_ bool fOwnsBuffer);

// Reference count implementation

    XUINT32 AddRef()  const final {return CInterlockedReferenceCount::AddRef();}
    XUINT32 Release() const final {return CInterlockedReferenceCount::Release();}

    // IPALMemory implementation

    XUINT32 GetSize()    const final  { return m_cBuffer; }
    void   *GetAddress() const final { return (void*)m_pBuffer; }

private:
    XUINT32          m_cBuffer;
    _Field_size_opt_(m_cBuffer) XUINT8          *m_pBuffer;
    bool            m_fOwnsBuffer;
};


//------------------------------------------------------------------------
//
//  Class:  CWinFile
//
//  Synopsis:
//      The Windows implementation of the PAL file object.
//
//------------------------------------------------------------------------

class CWinFile final : public IPALFile
{
public:
    CWinFile();
   ~CWinFile();

// IPALWaitable methods

    _Check_return_ HRESULT Close() final;
    XHANDLE GetHandle() final;

// IPALFile methods

    XUINT32 Release() override;
    _Check_return_ HRESULT Read(_In_ XUINT32 cBuffer, _Out_writes_bytes_(cBuffer) void *pBuffer, _Out_opt_ XUINT32 *pcRead) final;
    _Check_return_ HRESULT Write(_In_ XUINT32 cBuffer, _In_reads_bytes_(cBuffer) const void *pBuffer, _Out_opt_ XUINT32 *pcWritten) final;
    _Check_return_ HRESULT SetFilePointer(_In_ XINT64 cbOffset, _In_ XINT32 nMoveMethod) final;
    _Check_return_ HRESULT SetEndOfFile() final;
    _Check_return_ HRESULT GetSize(_Out_ XUINT64 *pSize) final;
    _Check_return_ HRESULT MapRange(_In_ XUINT64 nOffset, _In_ XUINT32 cRange, _Outptr_ IPALMemory **ppMemory) final;
    _Check_return_ HRESULT GetLastModifiedTime(_Out_ XINT32* pLastModified) final;

// CWinFile methods

    XHANDLE GetFileHandle();

// CWinFile members

    XUINT32 m_cRef;
    HANDLE  m_hSystem;
    UINT64  m_cSize;
};

//------------------------------------------------------------------------
//
//  Class:  CWinClock
//
//  Synopsis:
//      Get at platform clock
//
//------------------------------------------------------------------------

class CWinClock final : public CXcpObjectBase<IPALClock>
{
public:
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IPALClock>);

    static _Check_return_ HRESULT Create(_Outptr_ CWinClock **ppClock);

    // IPALClock overrides
    XDOUBLE GetAbsoluteTimeInSeconds() final;

private:
    CWinClock();

private:
// Variables for tracking the clock state

    LARGE_INTEGER m_lTimeStart;
    LARGE_INTEGER m_lFreq;

#if XCP_MONITOR
    // CWinClock cannot use the debug heap allocation APIs as they
    // indirectly use WinClock to obtain timestamps for debug messages.
    __declspec(allocator) void *__cdecl operator new(size_t cSize)
    {
        return HeapAlloc(GetProcessHeap(), 0, cSize);
    }
    void __cdecl operator delete(void *pAddress)
    {
        HeapFree(GetProcessHeap(), 0, pAddress);
    }
#endif
};

//------------------------------------------------------------------------
//
//  Interface:  CWinApplicationSingleton
//
//  Synopsis:
//      Control object used in the construction/deconstruction
//  of singletons.
//
//------------------------------------------------------------------------

class CWinApplicationSingleton final : IPALApplicationSingleton
{
private:
    CWinApplicationSingleton(XHANDLE hMutex);
    ~CWinApplicationSingleton();
public:
    static _Check_return_ HRESULT Create(_Outptr_ IPALApplicationSingleton **ppSingleton);
    _Success_(this->m_hMutex != NULL) _Releases_lock_(this->m_hMutex) XUINT32 Release() final;

private:
    XHANDLE m_hMutex;
};

//------------------------------------------------------------------------
//
//  Class:  CWinOSHandle
//
//  Synopsis:
//      Basic OS handle for Windows.
//
//------------------------------------------------------------------------

class CWinOSHandle final : public IPALWaitable
{
public:
    CWinOSHandle(HANDLE h) { m_handle = h; }

    _Check_return_ HRESULT Close() override
    {
        HRESULT hr = S_OK;

        if (m_handle)
        {
            CloseHandle(m_handle);
            m_handle = 0;
        }
        else
            hr = E_UNEXPECTED;

        delete this;

        return hr;
    }

    XHANDLE GetHandle() override
    {
        return m_handle;
    }

protected:
    HANDLE  m_handle;
};

//------------------------------------------------------------------------
//
//  Class:  CWinEvent
//
//  Synopsis:
//      Signable event for Windows.
//
//------------------------------------------------------------------------

class CWinEvent final : public IPALEvent
{
public:
    CWinEvent(HANDLE h) { m_handle = h; }

    _Check_return_ HRESULT Close() final
    {
        HRESULT hr = S_OK;

        if (m_handle)
        {
            CloseHandle(m_handle);
            m_handle = 0;
        }
        else
            hr = E_UNEXPECTED;

        delete this;

        return hr;
    }

    _Check_return_ HRESULT Set() final
    {
        return ::SetEvent(m_handle) ? S_OK : E_UNEXPECTED;
    }

    _Check_return_ HRESULT Reset() final
    {
        return ::ResetEvent(m_handle) ? S_OK : E_UNEXPECTED;
    }

    XHANDLE GetHandle() final
    {
        return m_handle;
    }

protected:
    HANDLE  m_handle;
};

struct QPACKET
{
    struct QPACKET *pNext;
    void *pData;
};

//------------------------------------------------------------------------
//
//  Class:  CWinQueue
//
//  Synopsis:
//      Waitable queue for Windows.
//
//------------------------------------------------------------------------

class CWinQueue final : public IPALQueue
{
public:
    CWinQueue()
    {
        m_handle = NULL;
        m_pHead = NULL;
        m_pTail = NULL;
    }

    _Check_return_ HRESULT Initialize()
    {
        HRESULT hr = S_OK;

        if (!InitializeCriticalSectionAndSpinCount(&m_cs, 0x80000001))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (NULL == (m_handle = ::CreateEvent(NULL, FALSE, FALSE, NULL)))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

    Cleanup:
        return hr;
    }

    _Check_return_ HRESULT Close() final
    {
        HRESULT hr = S_OK;
        QPACKET *next;

        if (m_handle)
        {
            CloseHandle(m_handle);
            m_handle = 0;
        }
        else
            hr = E_UNEXPECTED;

        while (m_pHead)
        {
            next = m_pHead->pNext;
            delete m_pHead;
            m_pHead = next;
        }

        DeleteCriticalSection(&m_cs);

        delete this;

        return hr;
    }

    void Post(_In_ void *pAddress) final
    {
        QPACKET *curr;
        XINT32  bAlert;

    // Do the 'new' outside of the critical section

        curr = new QPACKET;
        curr->pNext = NULL;
        curr->pData = pAddress;

        EnterCriticalSection(&m_cs);

        if (m_pTail)
        {
            bAlert = FALSE;
            m_pTail->pNext = curr;
            m_pTail = curr;
        }
        else
        {
        // The head pointer will always be NULL when the tail pointer
        // is NULL.  See the logic/comment in CWinQueue::Get

            bAlert = TRUE;
            m_pHead = curr;
            m_pTail = curr;
        }

        if (bAlert)
            ::SetEvent(m_handle);

        LeaveCriticalSection(&m_cs);
    }

    _Check_return_ HRESULT Get(_Outptr_ void **ppAddress, _In_ XUINT32 nWait) final
    {
        HRESULT hr = S_OK;
        QPACKET *curr = NULL;

        EnterCriticalSection(&m_cs);

        if (!m_pHead)
        {
            LeaveCriticalSection(&m_cs);

            if (WAIT_OBJECT_0 != WaitForSingleObject(m_handle, nWait))
            {
                return E_UNEXPECTED;
            }

            EnterCriticalSection(&m_cs);
        }

        if (!m_pHead)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

    // Get the first item in the queue, extract the data address and
    // advance the queue position.  Make sure that if the queue empties
    // the tail pointer is updated correctly.

        curr = m_pHead;
       *ppAddress = curr->pData;
        m_pHead = m_pHead->pNext;
        if (!m_pHead)
        {
            // Reset the event since queue is empty.
            // This is redundant if we came via WaitForSingleObject codepath.
            // However, if there was already a single item when this method is called,
            // and we don't reset the event, we will leave the event in a signaled
            // state with no items in the queue.
            m_pTail = NULL;
            ::ResetEvent(m_handle);
        }

    Cleanup:
        LeaveCriticalSection(&m_cs);
        delete curr;

        return hr;
    }

    _Check_return_ HRESULT Remove(_In_ void *pAddress) final
    {
        auto lock = wil::EnterCriticalSection(&m_cs);

        QPACKET* previous = nullptr;
        for (QPACKET* current = m_pHead; current != nullptr; current = current->pNext)
        {
            if (current->pData == pAddress)
            {
                if (previous != nullptr)
                {
                    previous->pNext = current->pNext;
                }
                else
                {
                    m_pHead = current->pNext;
                }

                if (m_pTail == current)
                {
                    m_pTail = previous;
                }

                delete current;

                if (m_pHead == nullptr)
                {
                    ::ResetEvent(m_handle);
                }

                return S_OK;
            }
            previous = current;
        }

        return E_FAIL;
    }

    XHANDLE GetHandle() final
    {
        return m_handle;
    }

protected:
    HANDLE   m_handle;          // An event (only used when queue is empty)
    _Guarded_by_(m_cs) QPACKET *m_pHead;           // First item in the queue
    _Guarded_by_(m_cs) QPACKET *m_pTail;           // Final item in the queue
    CRITICAL_SECTION m_cs{};      // Synchronization object
};

#if XCP_MONITOR

//------------------------------------------------------------------------
//
//  Class:  CWinThreadMonitor
//
//  Synopsis:
//      Per-thread debug monitoring data
//
//------------------------------------------------------------------------

class CWinThreadMonitor : public IThreadMonitor
{
public:
    // CWinThreadMonitor must not use the debug heap allocation APIs as
    // they indirectly call back into the thread monitor. Provide direct allocation
    // from the Win32 Heap.
    __declspec(allocator) void *__cdecl operator new(size_t cSize, int, int)
    {
        return HeapAlloc(GetProcessHeap(), 0, cSize);
    }

    CWinThreadMonitor() {m_clocksWhileSuspended = 0; m_threadId = 0;}
    ~CWinThreadMonitor() {}
    void Suspend() override;
    void Resume() override;
};

struct CCallingMethod : public ICallingMethod
{
    __declspec(allocator) void *__cdecl operator new[](size_t cSize)
    {
        return HeapAlloc(GetProcessHeap(), 0, cSize);
    }
    void __cdecl operator delete[](void *pAddress)
    {
        HeapFree(GetProcessHeap(), 0, pAddress);
    }
};

#endif

//------------------------------------------------------------------------
//
//  Class:  CWinRenderCounters
//
//  Synopsis:
//      Render counters
//
//------------------------------------------------------------------------
struct RenderCounters;
class CWinRenderCounters final : public IXcpRenderCounters
{
public:
    CWinRenderCounters() :
        m_pCounters(NULL), m_hMapFile(HANDLE(0))
    { }
    static HRESULT Create(_Outptr_ IXcpRenderCounters **ppRenderCounters);
    HRESULT Increment(_In_ XUINT32 uCounter) override;
    ~CWinRenderCounters() override;

private:
    RenderCounters *m_pCounters;
    HANDLE m_hMapFile;
};

//------------------------------------------------------------------------
//
//  Class:  CWindowsServices
//
//  Synopsis:
//      The Windows implementation of the class to back up the IPlatformServices
//  interface.
//
//------------------------------------------------------------------------
class CWinPrintingData;

class CWindowsServices final : public IPlatformServices
{
#ifdef STERLING_TESTS
friend class TestWindowsServices;
#endif
public:
    CWindowsServices();
    ~CWindowsServices();

    // CWindowsServices itself cannot use the platform services memory allocation
    // API as that relies on gps already being ready. Provide direct allocation
    // from the Win32 Heap.
    __declspec(allocator) void *__cdecl operator new(size_t cSize)
    {
        return HeapAlloc(GetProcessHeap(), 0, cSize);
    }

    // In deleting an instance of CWindowsServices (expected to be the one that gps
    // is pointing to) a call to gps->OSMemoryFree() will fail, so
    // it is necessary to use a CMacServices specific operator delete
    void operator delete(void * pAddress)
    {
        HeapFree(GetProcessHeap(), 0, pAddress);
    }


    // Debugging support
    void InitDebugTrace() override;
    void CleanupDebugTrace() override;
    bool IsDebugTraceTypeActive(_In_ XDebugTraceType traceType) override;
    _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, ...) override;
    _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, _In_ va_list args) override;
    _Check_return_ HRESULT DebugOutputSzNoEndl(_In_z_ const WCHAR *pFormattedMsg, ...) override;
                  void               DebugOutputSz(_In_z_ const WCHAR *pString) override;

#if DBG
    _Check_return_ bool              DebugAssertMessageBox(
                                                _In_z_ const WCHAR *pCondition,
                                                _In_z_ const WCHAR *pFile,
                                                XUINT32 nLine) override;

    bool TESTSignalMemLeak() override;

    void TESTSignalManagedMemLeak(_Out_ bool * showAssert) override;

    void TESTSignalASSERT(_In_z_ const WCHAR *pString) override;

    void TESTSignalReferenceTrackerLeak() override;

    bool IsStressModeEnabled() override;

    XUINT32 GetStressLeakThreshold() override;

#ifdef TRACK_LOCKS
    virtual void RecordLock(void *);
    virtual void RemoveLock(void *);
    virtual void CheckLocks();
#endif

#endif

#if XCP_MONITOR
                  void            YieldSlice() override;
    _Check_return_ HRESULT         GetMonitorBuffer(_Outptr_ MonitorBuffer **ppMonitorMemory) override;
    _Check_return_ HRESULT         ReleaseMonitorBuffer(_In_ MonitorBuffer *pMonitorBuffer) override;
    _Check_return_ IThreadMonitor *GetThreadMonitor() override;
                  void            InitializeThreadMonitoring() override;
                  void            DeleteThreadMonitoring() override;
                  HRESULT         StringCchVPrintfW(WCHAR *pString, XUINT32 cString, const WCHAR *pFormat, char *pVArgs) override;
                  HRESULT         StringCchVPrintfA(char *pString, XUINT32 cString, const char *pFormat, char *pVArgs) override;
                  XINT32           YesNoMessageBox(WCHAR *pContent, const WCHAR *pTitle) override;

    bool XcpVTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        void     *pVArgs          // Var args
    ) override;

    bool XcpTrace(
        XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
        const WCHAR   *pFilename,
        XINT32   iLine,
        XINT32   iValue,          // E.g. assertion value, HRESULT
        const WCHAR    *pTestString,    // E.g. Assertion string
        const WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
        ...                       // Var args
    ) override;
    
#pragma region // Methods to proxy the trace messages to somewhere else.

#if DBG
    void SetTraceMessageSink(IXcpTraceMessageSink* pTraceSink) override
    {
        // If xcpmon is attached, we'll use it's flag settings. Other wise we set a default.
        if (XcpTraceMonitorAttached())
        {
            // TODO: Can do a getSettings from the sink?
           *m_pTraceFlags = XUINT32(TraceAlways | TraceIfc | TraceAllocationStack | TraceLeakAddresses | TraceLeakTopLevel);
        }
        m_pTraceSink = pTraceSink;
    }
    HRESULT GetTraceMessageSink(IXcpTraceMessageSink** ppTraceSink) override { *ppTraceSink = m_pTraceSink; return S_OK; }
#endif //DBG

    void XcpEnterSection(XUINT16 id) override;
    void XcpLeaveSection(XUINT16 id) override;
    void XcpPopToMark() override;

#if DBG
    HRESULT XcpTraceMonitorInitialize(XUINT8 testMode) override;
#else
    virtual HRESULT XcpTraceMonitorInitialize();
#endif
    void    XcpTraceMonitorShutdown() override;
    XINT32  XcpTraceMonitorAttached() override;

    _Check_return_ HRESULT CaptureStack(
        _In_                            XUINT32    cMaxCallers,
        _Out_                           XUINT32   *pcCallers,
        _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers,
        _In_                            XUINT32    cIgnoreLevels
    ) override;

    _Check_return_ HRESULT GetCallerSourceLocations(
        _In_                         XUINT32          cCallers,
        _In_reads_(cCallers)        XUINT64         *pCallers,
        _Outptr_result_buffer_(cCallers) ICallingMethod **ppCallingMethods  // Receives pointer to array of cCallers ICallingMethods
    ) override;

    void FreeCallingMethods(ICallingMethod *pCallers) override;

#endif // #if XCP_MONITOR

    _Check_return_ HRESULT XcpRenderCountersInitialize() override;
    _Check_return_ HRESULT XcpRenderCountersIncrement(_In_ XUINT32 uCounter) override;

    // trace flag settings
    void SetTraceFlags(XUINT32* pFlags) override
    {
        *m_pTraceFlags = *pFlags;
    }

    XUINT32 GetTraceFlags() override
    {
        return *m_pTraceFlags;
    };

#pragma endregion

// File services

    _Check_return_ HRESULT FileCreate(_In_ XUINT32 cName, _In_reads_(cName) const WCHAR *pName, _In_ XINT32 fileOptions, _Outptr_ IPALFile **ppFile) override;

    HRESULT CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Outptr_ IPALStream **ppPALStream) override;

// Processwide control mechanism

    _Check_return_ HRESULT GetApplicationSingleton(_Outptr_ IPALApplicationSingleton **ppSingleton) override;

// URI services

    _Check_return_ HRESULT UriCreate(
                                  _In_ XUINT32 cString,
                                  _In_reads_(cString) const WCHAR *pString,
                                  _Outptr_ IPALUri **ppUri) override;

// Math and format conversion services

    _Check_return_ HRESULT CreateRegion(_Outptr_ IPALRegion **ppRegion) override;

// Safe print functions

    _Check_return_ HRESULT PrintStringCchW(
                                    _Out_writes_(cchDest) WCHAR *pszDest,
                                    _In_ XUINT32 cchDest,
                                    _In_z_ const WCHAR *pszFormat,
                                    ...) override;

    _Check_return_ HRESULT PrintStringCchExW(
                                    _Out_writes_(cchDest) WCHAR *pszDest,
                                    _In_ XUINT32 cchDest,
                                    _Outptr_opt_result_buffer_(*pcchRemaining) WCHAR **ppszDestEnd,
                                    _Out_opt_ XUINT32 *pcchRemaining,
                                    _In_ XUINT32 dwFlags,
                                    _In_z_ const WCHAR *pszFormat,
                                    ...) override;

// Threading and synchronization services

    _Check_return_ HRESULT ThreadCreate(
        _Outptr_ IPALWaitable **ppThread,
        _In_ PALTHREADPFN pfn,
        _In_ XUINT32 cData,
        _In_reads_(cData) XUINT8 *pData) override;

    _Check_return_ HRESULT EventCreate(
        _Outptr_ IPALEvent **ppEvent,
        _In_ XINT32 bSignaled,
        _In_ XINT32 bManual) override;

    _Check_return_ HRESULT QueueCreate(
        _Outptr_ IPALQueue **ppQueue) override;

    _Check_return_ HRESULT NamedEventCreate(
        _Outptr_ IPALEvent** ppEvent,
        PAL_IPCInitMode initMode,
        bool bInitialState,
        bool bManualReset,
        _In_ const xstring_ptr_view& strName,
        bool bReturnFailureIfCreationFailed) override;

    XUINT32 WaitForObjects(
        _In_ XUINT32 cWaitable,
        _In_reads_(cWaitable) IPALWaitable **ppWaitable,
        _In_ XINT32 bAll,
        _In_ XUINT32 nTimeout) override;

    XINT32  ThreadGetPriority(_In_opt_ IPALWaitable *pThread) override;
    HRESULT ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority) override;
    XINT32  ProcessGetPriority() override;
    HRESULT ProcessSetPriority(_In_ XINT32 nPriority) override;

    _Check_return_ HRESULT CreateWorkItemFactory(
        _Outptr_ IPALWorkItemFactory **ppWork) override;

// Font and text services

    // Provides platform's Font and Script Services.
    _Check_return_ HRESULT CreateFontAndScriptServices(_Outptr_ IPALFontAndScriptServices** ppFontAndScriptServices) override;

// Keyboard states
    _Check_return_ HRESULT GetKeyboardModifiersState( _Out_ XUINT32 *pModifiers) override;

// Time services

    _Check_return_ HRESULT CreateClock(_Outptr_ IPALClock **ppIClock) override;
    XUINT64 GetCPUTime() override;
    XUINT64 GetCPUMilliseconds() override;

// CPU Info

    _Check_return_ HRESULT GetNumberOfCPUCores(_Out_ XUINT32 *pcProcessorCores) override;
    bool IsSupportedPlatform() override;

// COM initialization
    _Check_return_ HRESULT CallCoInitializeMTA() override;
    void CallCoUninitialize() override;

// Performance measurement

    _Check_return_ HRESULT XcpPerfMarker(_In_ XUINT64 lMarker) override;
    _Check_return_ HRESULT XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, _In_ WCHAR* szMarkerInfo) override;
    _Check_return_ HRESULT XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, _In_ DWORD nMarkerInfo) override;
    virtual _Check_return_ HRESULT SendXcpPerfMarker(_In_ XUINT64 lMarker, _In_ HWND hTargetWindow);
    PerfMarkerType PerfMarkersEnabled() override;

// Window services

    _Check_return_ HRESULT BrowserHostCreate(_In_ IXcpHostSite *pSite, _In_ IXcpDispatcher *pDispatcher, _Outptr_ IXcpBrowserHost ** ppHost) override;

    _Check_return_ HRESULT GetPlatformUtilities(_Outptr_ IPlatformUtilities** ppInterface ) override;

    // Touch interaction
    _Check_return_ HRESULT IsDirectManipulationSupported(_Out_ bool &isDirectManipulationSupported) override;

    _Check_return_ HRESULT GetDirectManipulationService(std::shared_ptr<DirectManipulationServiceSharedState> sharedState, _Outptr_ IPALDirectManipulationService** ppDirectManipulationService) override;

    // InputPane Service
    _Check_return_ HRESULT GetInputPaneInteraction(
        _In_ IXcpInputPaneHandler *pInputPaneHandler,
        _Outptr_ IPALInputPaneInteraction** ppInputPaneInteraction) override;

// Printing Services
    virtual _Check_return_ HRESULT PrintPage(_In_ void* pvBitmap, _In_ IPALPrintingData* pPalPD);

    _Check_return_ HRESULT CreateD2DPrintFactoryAndTarget(
        _Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory,
        _Outptr_ IPALPrintTarget** ppPALPrintTarget) override;

    _Check_return_ HRESULT CreateD2DPrintingData(
        _Outptr_ IPALD2DPrintingData** ppPrintingData) override;

    XUINT32 GenerateSecurityToken() override;

    XUINT32 /* bool */ PerformanceFrequency(XINT64_LARGE_INTEGER *lpFrequency) override;
    XUINT32 /* bool */ PerformanceCounter(XINT64_LARGE_INTEGER *lpPerformanceCount) override;

    XUINT64 GetMemoryCount() override;

    _Check_return_ HRESULT CreateGraphicsDeviceManager(_Outptr_ WindowsGraphicsDeviceManager **ppIGraphicsDeviceManager) override;

    _Check_return_ HRESULT CreateResourceProvider(_In_ CCoreServices *pCore, _Outptr_ IPALResourceProvider **ppResourceProvider) override;

    _Check_return_ HRESULT CreateApplicationDataProvider(_Outptr_ IPALApplicationDataProvider **ppAppDataProvider) override;

    _Check_return_ HRESULT GetFadeInThemeAnimationData( _Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms ) override;
    _Check_return_ HRESULT GetFadeOutThemeAnimationData( _Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms ) override;

    _Check_return_ HRESULT CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Outptr_ IPALMemory** ppPALMemory) override;

// Remote desktop
    _Check_return_ HRESULT IsOnRemoteDesktopSession( _Out_ bool* pfOnRemoteDesktop ) override;

#if XCP_MONITOR
    // Check if AG_MPStress is defined in the process environment
    void GetTrackerStressFromEnvironment( _Out_ int *maxIterations, _Out_ int *startIteration, _Out_ bool *backgroundGC ) override;
#endif


    // IPALProcessCharacteristics
    bool IsProcessPackaged() override;
    bool IsProcessAppContainer() override;
    _Check_return_ HRESULT GetProcessPackagePath(_Out_ xstring_ptr* pstrPackagePath) override;
    _Check_return_ HRESULT GetProcessPackageFullName(_Out_ xstring_ptr* pstrPackageFullName) override;
    _Check_return_ HRESULT GetProcessModernAppId(_Out_ xstring_ptr* pstrAppId) override;
    _Check_return_ HRESULT GetProcessImageName(_Out_ xstring_ptr* pstrImageName) override;

    // IPALAutomationServices
    _Check_return_ HRESULT GetAutomationProvider(
        _In_ IXcpHostSite *pSite,
        _In_ CDependencyObject *pElement,
        _Outptr_ IUnknown** ppProvider) override;

private:

//private static

    _Check_return_ static HRESULT InitializeResourceDll();

protected:

    bool AreMemoryPressureCallbacksRegistered();

private:

    wrl::ComPtr<ixp::IInputKeyboardSourceStatics> m_keyboardInputStatics;

    double  m_eFrequency;
    PerfMarkerType m_perfMarkerType;
    XINT32 m_perfCheckCompleted;
    HWND m_hPerfMonitor;
    HWND m_hBrowserControl;

// To proxy trace messages to somewhere else.
#if XCP_MONITOR && DBG
    IXcpTraceMessageSink* m_pTraceSink;
#endif XCP_MONITOR
    XUINT32              *m_pTraceFlags;

    IXcpRenderCounters *m_pRenderCounters;

    IValueStore*    pHostProps{};

    bool m_isDirectManipulationSupported;
    bool m_hasCheckedForDirectManipulationSupport;
};

// Private messages processed by Dispatcher.
//
// Update WM_FIRST & WM_LAST when this list is changed
#define WM_XCP_ERROR_MSG    (WM_USER + 1)
#define WM_INTERNAL_TICK    (WM_USER + 2)
#define WM_SCRIPT_CALL_BACK (WM_USER + 3)
#define WM_APPLICATION_STARTUP_EVENT_COMPLETE (WM_USER + 6)
#define WM_FORCE_GC_COLLECT_EVENT (WM_USER + 7)
#define WM_DOWNLOAD_ASYNC_REQUESTS (WM_USER + 8)
#define WM_SCRIPT_SYNC_CALL_BACK (WM_USER + 12)
#define WM_SCRIPT_SYNC_EFFECTIVEVIEWPORTCHANGED (WM_USER + 13)
#define WM_MINVER_NOTIFICATION (WM_USER + 14)
#define WM_EXECUTE_ON_UI_CALLBACK (WM_USER + 15)
#define WM_EXECUTE_ON_UI_CALLBACK_ALLOW_REENTRANCY (WM_USER + 16)
#define WM_UNUSED_USER_ENTRY_18 (WM_USER + 18)
#define WM_DOCUMENTPRINT (WM_USER + 20)
#define WM_SCRIPT_SYNC_INPUT_CALL_BACK (WM_USER + 21)
#define WM_SCRIPT_SYNC_LOADED_CALL_BACK (WM_USER + 22)
#define WM_REPLAY_PREVIOUS_POINTERUPDATE (WM_USER + 23)
#define WM_SYNC_CHANGING_FOCUS_CALLBACK (WM_USER + 24)
#define WM_SCRIPT_SYNC_ALLOW_REENTRANCY_CALL_BACK (WM_USER + 25)
#define WM_EXECUTE_ON_UI_CALLBACK_BLOCK_REENTRANCY (WM_USER + 26)
// Update WM_FIRST & WM_LAST when this list is changed
#define WM_FIRST WM_XCP_ERROR_MSG
#define WM_LAST WM_EXECUTE_ON_UI_CALLBACK_BLOCK_REENTRANCY




#define WIN32_SCROLLBAR                 0
#define WIN32_BACKGROUND                1
#define WIN32_ACTIVECAPTION             2
#define WIN32_INACTIVECAPTION           3
#define WIN32_MENU                      4
#define WIN32_WINDOW                    5
#define WIN32_WINDOWFRAME               6
#define WIN32_MENUTEXT                  7
#define WIN32_WINDOWTEXT                8
#define WIN32_CAPTIONTEXT               9
#define WIN32_ACTIVEBORDER              10
#define WIN32_INACTIVEBORDER            11
#define WIN32_APPWORKSPACE              12
#define WIN32_HIGHLIGHT                 13
#define WIN32_HIGHLIGHTTEXT             14
#define WIN32_BTNFACE                   15
#define WIN32_BTNSHADOW                 16
#define WIN32_GRAYTEXT                  17
#define WIN32_BTNTEXT                   18
#define WIN32_INACTIVECAPTIONTEXT       19
#define WIN32_BTNHIGHLIGHT              20
#define WIN32_3DDKSHADOW                21
#define WIN32_3DLIGHT                   22
#define WIN32_INFOTEXT                  23
#define WIN32_INFOBK                    24
#define WIN32_HOTLIGHT                  26
#define WIN32_GRADIENTACTIVECAPTION     27
#define WIN32_GRADIENTINACTIVECAPTION   28
#define WIN32_MENUHILIGHT               29
#define WIN32_MENUBAR                   30

#define WIN32_DESKTOP                   WIN32_BACKGROUND
#define WIN32_ACTIVECAPTIONTEXT         WIN32_CAPTIONTEXT;
#define WIN32_CONTROL                   WIN32_BTNFACE;
#define WIN32_CONTROLDARK               WIN32_BTNSHADOW;
#define WIN32_CONTROLDARKDARK           WIN32_3DDKSHADOW;
#define WIN32_CONTROLLIGHT              WIN32_3DLIGHT;
#define WIN32_CONTROLLIGHTLIGHT         WIN32_BTNHIGHLIGHT;
#define WIN32_CONTROLTEXT               WIN32_BTNTEXT;
#define WIN32_HOTTRACK                  WIN32_HOTLIGHT;
#define WIN32_INFO                      WIN32_INFOBK;
#define WIN32_MENUHIGHLIGHT             WIN32_MENUHILIGHT;

// Timer Id Definition for Windows Platform
#define TOUCH_INTERACTION_INERTIA_TIMER_ID 58

