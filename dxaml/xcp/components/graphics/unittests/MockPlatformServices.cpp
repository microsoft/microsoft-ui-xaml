// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MockPlatformServices.h"

// MockServiceObjectBase - Base class for all mock Platform objects.

template<class T>
class MockServiceObjectBase : public T
{
public:
    MockServiceObjectBase() : m_refCount(1)
    {
    }

    virtual ~MockServiceObjectBase()
    {
    }

    virtual XUINT32 AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    virtual XUINT32 Release()
    {
        ULONG newRefCount = InterlockedDecrement(&m_refCount);
        if (newRefCount == 0)
        {
            delete this;
        }
        return newRefCount;
    }

private:
    ULONG m_refCount;
};

// Clock Mock

class MockPlatformClock : public MockServiceObjectBase<IMockPlatformClock>
{
public:
    MockPlatformClock() : m_currentTimeInSeconds(0)
    {
    }

    virtual double GetAbsoluteTimeInSeconds()
    {
        return m_currentTimeInSeconds;
    }

    virtual void SetAbsoluteTimeInSeconds(double time)
    {
        m_currentTimeInSeconds = time;
    }

    virtual void AddSeconds(double seconds)
    {
        m_currentTimeInSeconds += seconds;
    }

private:
    double m_currentTimeInSeconds;
};

// PlatformServices Mock

class MockPlatformServices : public IMockPlatformServices
{
private:
    xref_ptr<MockPlatformClock> m_clock;

public:

    MockPlatformServices()
    {
        m_clock.attach(new MockPlatformClock());
    }

    ~MockPlatformServices()
    {
    }

    virtual void GetMockClock(IMockPlatformClock** clock)
    {
        xref_ptr<IMockPlatformClock> c(m_clock);
        *clock = c.detach();
    }

    //  IPALDebuggingServices
#if XCP_MONITOR
    virtual bool XcpVTrace(XUINT32 uClass, const WCHAR *pFilename, XINT32 iLine, XINT32 iValue, const WCHAR *pTestString, const WCHAR *pMessageString, void *pVArgs)
    {
        UNREFERENCED_PARAMETER(uClass);
        UNREFERENCED_PARAMETER(pFilename);
        UNREFERENCED_PARAMETER(iLine);
        UNREFERENCED_PARAMETER(iValue);
        UNREFERENCED_PARAMETER(pTestString);
        UNREFERENCED_PARAMETER(pMessageString);
        UNREFERENCED_PARAMETER(pVArgs);
        VERIFY_FAIL();
        return false;
    }

    virtual bool XcpTrace(XUINT32 uClass, const WCHAR *pFilename, XINT32 iLine, XINT32 iValue, const WCHAR *pTestString, const WCHAR *pMessageString, ...)
    {
        UNREFERENCED_PARAMETER(uClass);
        UNREFERENCED_PARAMETER(pFilename);
        UNREFERENCED_PARAMETER(iLine);
        UNREFERENCED_PARAMETER(iValue);
        UNREFERENCED_PARAMETER(pTestString);
        UNREFERENCED_PARAMETER(pMessageString);
        VERIFY_FAIL();
        return false;
    }
#endif // end #if XCP_MONITOR

#if XCP_MONITOR

    virtual void YieldSlice()
    {
        VERIFY_FAIL();
    }

    virtual _Check_return_ HRESULT GetMonitorBuffer(_Outptr_ MonitorBuffer **ppMonitorMemory)
    {
        UNREFERENCED_PARAMETER(ppMonitorMemory);
        return E_NOTIMPL;
    }

    virtual _Check_return_ IThreadMonitor *GetThreadMonitor()
    {
        VERIFY_FAIL();
        return nullptr;
    }

    virtual _Check_return_ HRESULT ReleaseMonitorBuffer(_In_ MonitorBuffer *pMonitorBuffer)
    {
        UNREFERENCED_PARAMETER(pMonitorBuffer);
        return E_NOTIMPL;
    }

    virtual void InitializeThreadMonitoring()
    {
        VERIFY_FAIL();
    }

    virtual void DeleteThreadMonitoring()
    {
        VERIFY_FAIL();
    }

    virtual HRESULT StringCchVPrintf(WCHAR *pString, XUINT32 cString, const WCHAR *pFormat, char *pVArgs)
    {
        UNREFERENCED_PARAMETER(pString);
        UNREFERENCED_PARAMETER(cString);
        UNREFERENCED_PARAMETER(pFormat);
        UNREFERENCED_PARAMETER(pVArgs);
        return E_NOTIMPL;
    }

    virtual HRESULT StringCchVPrintfA(char *pString, XUINT32 cString, const char *pFormat, char *pVArgs)
    {
        UNREFERENCED_PARAMETER(pString);
        UNREFERENCED_PARAMETER(cString);
        UNREFERENCED_PARAMETER(pFormat);
        UNREFERENCED_PARAMETER(pVArgs);
        return E_NOTIMPL;
    }

    virtual XINT32 YesNoMessageBox(WCHAR *pContent, const WCHAR *pTitle)
    {
        UNREFERENCED_PARAMETER(pContent);
        UNREFERENCED_PARAMETER(pTitle);
        VERIFY_FAIL();
        return 0;
    }

    virtual void XcpEnterSection(XUINT16 id)
    {
        UNREFERENCED_PARAMETER(id);
        VERIFY_FAIL();
    }

    virtual void XcpLeaveSection(XUINT16 id)
    {
        UNREFERENCED_PARAMETER(id);
        VERIFY_FAIL();
    }

    virtual void XcpPopToMark()
    {
        VERIFY_FAIL();
    }

#if DBG
    virtual HRESULT XcpTraceMonitorInitialize(XUINT8 testMode)
    {
        UNREFERENCED_PARAMETER(testMode);
        return E_NOTIMPL;
    }
#else

    virtual HRESULT XcpTraceMonitorInitialize()
    {
        return E_NOTIMPL;
    }
#endif // #if DBG

    virtual void XcpTraceMonitorShutdown()
    {
        VERIFY_FAIL();
    }

    virtual XINT32 XcpTraceMonitorAttached()
    {
        VERIFY_FAIL();
        return 0;
    }

    // Stack trace support

    virtual _Check_return_ HRESULT CaptureStack(_In_ XUINT32 cMaxCallers, _Out_ XUINT32 *pcCallers, _Outptr_result_buffer_(cMaxCallers) XUINT64 **ppCallers, _In_ XUINT32 cIgnoreLevels)
    {
        UNREFERENCED_PARAMETER(cMaxCallers);
        UNREFERENCED_PARAMETER(pcCallers);
        UNREFERENCED_PARAMETER(ppCallers);
        UNREFERENCED_PARAMETER(cIgnoreLevels);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT GetCallerSourceLocations(_In_ XUINT32 cCallers, _In_reads_(cCallers) XUINT64 *pCallers, _Outptr_result_buffer_(cCallers) ICallingMethod **ppCallingMethods)
    {
        UNREFERENCED_PARAMETER(cCallers);
        UNREFERENCED_PARAMETER(pCallers);
        UNREFERENCED_PARAMETER(ppCallingMethods);
        return E_NOTIMPL;
    }

    virtual void FreeCallingMethods(ICallingMethod *pCallers)
    {
        UNREFERENCED_PARAMETER(pCallers);
        VERIFY_FAIL();
    }

#if DBG

#pragma region // Methods to support setting a proxy target that also gets the trace messages
    virtual void SetTraceMessageSink(IXcpTraceMessageSink* pTraceSink)
    {
        UNREFERENCED_PARAMETER(pTraceSink);
        VERIFY_FAIL();
    }

    virtual HRESULT GetTraceMessageSink(IXcpTraceMessageSink** ppTraceSink)
    {
        UNREFERENCED_PARAMETER(ppTraceSink);
        return E_NOTIMPL;
    }
#pragma endregion

#endif //DBG

#endif // #if XCP_MONITOR

    // Performance measurement

    virtual _Check_return_ HRESULT XcpPerfMarker(_In_ XUINT64 lMarker)
    {
        UNREFERENCED_PARAMETER(lMarker);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT XcpPerfMarkerStringInfo(_In_ XUINT64 lMarker, _In_z_ WCHAR* szMarkerInfo)
    {
        UNREFERENCED_PARAMETER(lMarker);
        UNREFERENCED_PARAMETER(szMarkerInfo);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT XcpPerfMarkerDwordInfo(_In_ XUINT64 lMarker, XDWORD nMarkerInfo)
    {
        UNREFERENCED_PARAMETER(lMarker);
        UNREFERENCED_PARAMETER(nMarkerInfo);
        return E_NOTIMPL;
    }

    virtual PerfMarkerType PerfMarkersEnabled()
    {
        VERIFY_FAIL();
        return (PerfMarkerType)0;
    }

    virtual _Check_return_ HRESULT XcpRenderCountersInitialize()
    {
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT XcpRenderCountersIncrement(_In_ XUINT32 uCounter)
    {
        UNREFERENCED_PARAMETER(uCounter);
        return E_NOTIMPL;
    }

    // Debugging support - services used by the debugging support code
    virtual void InitDebugTrace()
    {
        VERIFY_FAIL();
    }

    virtual void CleanupDebugTrace()
    {
        VERIFY_FAIL();
    }

    virtual bool IsDebugTraceTypeActive(_In_ XDebugTraceType traceType)
    {
        UNREFERENCED_PARAMETER(traceType);
        VERIFY_FAIL();
        return false;
    }

    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, ...)
    {
        UNREFERENCED_PARAMETER(traceType);
        UNREFERENCED_PARAMETER(pFormattedMsg);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT DebugTrace(_In_ XDebugTraceType traceType, _In_z_ const WCHAR *pFormattedMsg, _In_ va_list args)
    {
        UNREFERENCED_PARAMETER(traceType);
        UNREFERENCED_PARAMETER(pFormattedMsg);
        UNREFERENCED_PARAMETER(args);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT DebugOutputSzNoEndl(_In_z_ const WCHAR *pFormattedMsg, ...)
    {
        UNREFERENCED_PARAMETER(pFormattedMsg);
        return E_NOTIMPL;
    }

    virtual void DebugOutputSz(_In_z_ const WCHAR *pString)
    {
        UNREFERENCED_PARAMETER(pString);
        VERIFY_FAIL();
    }

#if DBG

    virtual _Check_return_ bool DebugAssertMessageBox(_In_z_ const WCHAR *pCondition, _In_z_ const WCHAR *pFile, XUINT32 nLine)
    {
        UNREFERENCED_PARAMETER(pCondition);
        UNREFERENCED_PARAMETER(pFile);
        UNREFERENCED_PARAMETER(nLine);
        VERIFY_FAIL();
        return false;
    }

    virtual bool TESTSignalMemLeak()
    {
        VERIFY_FAIL();
        return false;
    }

    virtual void TESTSignalManagedMemLeak(_Out_ bool * showAssert)
    {
        UNREFERENCED_PARAMETER(showAssert);
        VERIFY_FAIL();
    }

    virtual void TESTSignalASSERT(_In_z_ const WCHAR *pString)
    {
        UNREFERENCED_PARAMETER(pString);
        VERIFY_FAIL();
    }

    virtual void TESTSignalReferenceTrackerLeak()
    {
        VERIFY_FAIL();
    }

    virtual bool IsStressModeEnabled()
    {
        VERIFY_FAIL();
        return false;
    }

    virtual XUINT32 GetStressLeakThreshold()
    {
        VERIFY_FAIL();
        return 0;
    }

#endif // #if DBG

    // trace flag settings
    virtual void SetTraceFlags(XUINT32* pFlags)
    {
        UNREFERENCED_PARAMETER(pFlags);
        VERIFY_FAIL();
    }

    virtual XUINT32 GetTraceFlags()
    {
        VERIFY_FAIL();
        return 0;
    }

    virtual XUINT64 GetMemoryCount()
    {
        VERIFY_FAIL();
        return 0;
    }

    // Enable MPStress with an environment variable
#if XCP_MONITOR
    virtual void GetTrackerStressFromEnvironment(_Out_ int *maxIterations, _Out_ int *startIteration, _Out_ bool *backgroundGC)
    {
        UNREFERENCED_PARAMETER(maxIterations);
        UNREFERENCED_PARAMETER(startIteration);
        UNREFERENCED_PARAMETER(backgroundGC);
        VERIFY_FAIL();
    }
#endif

    // IPlatformServices
    virtual _Check_return_ HRESULT CreateGraphicsDeviceManager(_Outptr_ WindowsGraphicsDeviceManager **ppIGraphicsDeviceManager)
    {
        UNREFERENCED_PARAMETER(ppIGraphicsDeviceManager);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT BrowserHostCreate(_In_ IXcpHostSite *pSite, _In_ IXcpDispatcher *pDispatcher, _Outptr_ IXcpBrowserHost ** ppHost)
    {
        UNREFERENCED_PARAMETER(pSite);
        UNREFERENCED_PARAMETER(pDispatcher);
        UNREFERENCED_PARAMETER(ppHost);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateResourceProvider(_In_ CCoreServices *pCore, _Outptr_ IPALResourceProvider **ppResourceProvider)
    {
        UNREFERENCED_PARAMETER(pCore);
        UNREFERENCED_PARAMETER(ppResourceProvider);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateApplicationDataProvider(_Outptr_ IPALApplicationDataProvider **ppAppDataProvider)
    {
        UNREFERENCED_PARAMETER(ppAppDataProvider);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateWorkItemFactory(_Outptr_ IPALWorkItemFactory **ppWork)
    {
        UNREFERENCED_PARAMETER(ppWork);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateIPALStreamFromIPALMemory(_In_ IPALMemory *pPALMemory, _Outptr_ IPALStream **ppPALStream)
    {
        UNREFERENCED_PARAMETER(pPALMemory);
        UNREFERENCED_PARAMETER(ppPALStream);
        return E_NOTIMPL;
    }

    // IPALMemoryServices
    virtual _Check_return_ HRESULT CreatePALMemoryFromBuffer(_In_ XUINT32 cbBuffer, _In_reads_(cbBuffer) XUINT8* pBuffer, _In_ bool fOwnsBuffer, _Outptr_ IPALMemory** ppPALMemory)
    {
        UNREFERENCED_PARAMETER(cbBuffer);
        UNREFERENCED_PARAMETER(pBuffer);
        UNREFERENCED_PARAMETER(fOwnsBuffer);
        UNREFERENCED_PARAMETER(ppPALMemory);
        return E_NOTIMPL;
    }

    // IPALURIServices
    virtual _Check_return_ HRESULT UriCreate(_In_ XUINT32 cString, _In_reads_(cString) const WCHAR *pString, _Outptr_ IPALUri **ppUri)
    {
        UNREFERENCED_PARAMETER(cString);
        UNREFERENCED_PARAMETER(pString);
        UNREFERENCED_PARAMETER(ppUri);
        return E_NOTIMPL;
    }

    // IPALlMathServices
    virtual _Check_return_ HRESULT CreateRegion(_Outptr_ IPALRegion **ppRegion)
    {
        UNREFERENCED_PARAMETER(ppRegion);
        return E_NOTIMPL;
    }

    // IPALPrintIOServices
    virtual _Check_return_ HRESULT PrintStringCchW(_Out_writes_(cchDest) WCHAR *pszDest, _In_ XUINT32 cchDest, _In_z_ const WCHAR *pszFormat, ...)
    {
        UNREFERENCED_PARAMETER(pszDest);
        UNREFERENCED_PARAMETER(cchDest);
        UNREFERENCED_PARAMETER(pszFormat);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT PrintStringCchExW(_Out_writes_(cchDest) WCHAR *pszDest, _In_ XUINT32 cchDest, _Outptr_opt_result_buffer_(*pcchRemaining) WCHAR **ppszDestEnd, _Out_opt_ XUINT32 *pcchRemaining, _In_ XUINT32 dwFlags, _In_z_ const WCHAR *pszFormat, ...)
    {
        UNREFERENCED_PARAMETER(pszDest);
        UNREFERENCED_PARAMETER(cchDest);
        UNREFERENCED_PARAMETER(ppszDestEnd);
        UNREFERENCED_PARAMETER(pcchRemaining);
        UNREFERENCED_PARAMETER(dwFlags);
        UNREFERENCED_PARAMETER(pszFormat);
        return E_NOTIMPL;
    }

    // IPALThreadingServices
    virtual HRESULT ThreadCreate(_Outptr_ IPALWaitable **ppThread, _In_ PALTHREADPFN pfn, _In_ XUINT32 cData, _In_reads_(cData) XUINT8 *pData)
    {
        UNREFERENCED_PARAMETER(ppThread);
        UNREFERENCED_PARAMETER(pfn);
        UNREFERENCED_PARAMETER(cData);
        UNREFERENCED_PARAMETER(pData);
        return E_NOTIMPL;
    }

    virtual HRESULT EventCreate(_Outptr_ IPALEvent **ppEvent, _In_ XINT32 bSignaled, _In_ XINT32 bManual)
    {
        UNREFERENCED_PARAMETER(ppEvent);
        UNREFERENCED_PARAMETER(bSignaled);
        UNREFERENCED_PARAMETER(bManual);
        return E_NOTIMPL;
    }

    virtual HRESULT QueueCreate(_Outptr_ IPALQueue **ppQueue)
    {
        UNREFERENCED_PARAMETER(ppQueue);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT NamedEventCreate(_Outptr_ IPALEvent** ppEvent, PAL_IPCInitMode initMode, bool bInitialState, bool bManualReset, _In_ const xstring_ptr_view& strName, bool bReturnFailureIfCreationFailed)
    {
        UNREFERENCED_PARAMETER(ppEvent);
        UNREFERENCED_PARAMETER(initMode);
        UNREFERENCED_PARAMETER(bInitialState);
        UNREFERENCED_PARAMETER(bManualReset);
        UNREFERENCED_PARAMETER(strName);
        UNREFERENCED_PARAMETER(bReturnFailureIfCreationFailed);
        return E_NOTIMPL;
    }

    virtual XUINT32 WaitForObjects(_In_ XUINT32 cWaitable, _In_ IPALWaitable **ppWaitable, _In_ XINT32 bAll, _In_ XUINT32 nTimeout)
    {
        UNREFERENCED_PARAMETER(cWaitable);
        UNREFERENCED_PARAMETER(ppWaitable);
        UNREFERENCED_PARAMETER(bAll);
        UNREFERENCED_PARAMETER(nTimeout);
        VERIFY_FAIL();
        return 0;
    }

    virtual XINT32 ThreadGetPriority(_In_opt_ IPALWaitable *pThread)
    {
        UNREFERENCED_PARAMETER(pThread);
        VERIFY_FAIL();
        return 0;
    }

    virtual HRESULT ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority)
    {
        UNREFERENCED_PARAMETER(pThread);
        UNREFERENCED_PARAMETER(nPriority);
        return E_NOTIMPL;
    }

    virtual XINT32 ProcessGetPriority()
    {
        VERIFY_FAIL();
        return 0;
    }

    virtual HRESULT ProcessSetPriority(_In_ XINT32 nPriority)
    {
        UNREFERENCED_PARAMETER(nPriority);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateFontAndScriptServices(_Outptr_ PALText::IFontAndScriptServicesFactory** ppFontAndScriptServices)
    {
        UNREFERENCED_PARAMETER(ppFontAndScriptServices);
        return E_NOTIMPL;
    }

    // IPALCoreServices
    virtual _Check_return_ HRESULT FileCreate(_In_ XUINT32 cName, _In_reads_(cName) const WCHAR *pName, XINT32 fileOptions, _Outptr_ IPALFile **ppFile)
    {
        UNREFERENCED_PARAMETER(cName);
        UNREFERENCED_PARAMETER(pName);
        UNREFERENCED_PARAMETER(fileOptions);
        UNREFERENCED_PARAMETER(ppFile);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT GetPlatformUtilities(_Outptr_ IPlatformUtilities** ppInterface)
    {
        UNREFERENCED_PARAMETER(ppInterface);
        return E_NOTIMPL;
    }

    virtual XUINT64 GetCPUTime()
    {
        VERIFY_FAIL();
        return 0;
    }

    virtual XUINT64 GetCPUMilliseconds()
    {
        VERIFY_FAIL();
        return 0;
    }

    virtual _Check_return_ HRESULT CreateClock(_Outptr_ IPALClock **ppIClock)
    {
        m_clock->AddRef();
        *ppIClock = m_clock;
        return S_OK;
    }

    virtual _Check_return_ HRESULT GetNumberOfCPUCores(_Out_ XUINT32 *pcProcessorCores)
    {
        UNREFERENCED_PARAMETER(pcProcessorCores);
        return E_NOTIMPL;
    }

    virtual _Check_return_ bool IsSupportedPlatform()
    {
        VERIFY_FAIL();
        return false;
    }

    virtual _Check_return_ HRESULT GetApplicationSingleton(_Outptr_ IPALApplicationSingleton **ppSingleton)
    {
        UNREFERENCED_PARAMETER(ppSingleton);
        return E_NOTIMPL;
    }

    virtual XUINT32 GenerateSecurityToken()
    {
        VERIFY_FAIL();
        return 0;
    }

    virtual _Check_return_ HRESULT GetKeyboardModifiersState(_Out_ XUINT32 *pModifiers)
    {
        UNREFERENCED_PARAMETER(pModifiers);
        return E_NOTIMPL;
    }

    virtual XUINT32 /* bool */ PerformanceFrequency(_Out_ XINT64_LARGE_INTEGER *lpFrequency)
    {
        // Initialize the result to avoid uninitialized memory warning
        lpFrequency->QuadPart = 0;
        VERIFY_FAIL();
        return 0;
    }

    virtual XUINT32 /* bool */ PerformanceCounter(_Out_ XINT64_LARGE_INTEGER *lpPerformanceCount)
    {
        // Initialize the result to avoid uninitialized memory warning
        lpPerformanceCount->QuadPart = 0;
        VERIFY_FAIL();
        return 0;
    }

    virtual _Check_return_ HRESULT CallCoInitializeMTA()
    {
        return E_NOTIMPL;
    }

    virtual void CallCoUninitialize()
    {
        VERIFY_FAIL();
    }

    virtual _Check_return_ HRESULT IsOnRemoteDesktopSession(_Out_ bool* pfOnRemoteDesktop)
    {
        UNREFERENCED_PARAMETER(pfOnRemoteDesktop);
        return E_NOTIMPL;
    }

    // IPALTouchInteractionServices
    virtual _Check_return_ HRESULT IsDirectManipulationSupported(_Out_ bool &isDirectManipulationSupported)
    {
        UNREFERENCED_PARAMETER(isDirectManipulationSupported);
        return E_NOTIMPL;
    }

    virtual bool IsPointerInfoValid(_In_opt_ XHANDLE hWindow, _In_ XUINT32 pointerId)
    {
        UNREFERENCED_PARAMETER(hWindow);
        UNREFERENCED_PARAMETER(pointerId);
        VERIFY_FAIL();
        return false;
    }

    virtual _Check_return_ HRESULT GetDirectManipulationService(std::shared_ptr<DirectManipulationServiceSharedState> sharedState, _Outptr_ IPALDirectManipulationService** pDirectManipulationService)
    {
        UNREFERENCED_PARAMETER(sharedState);
        UNREFERENCED_PARAMETER(pDirectManipulationService);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT GetInputPaneInteraction(_In_ IXcpInputPaneHandler *pInputPaneHandler, _Outptr_ IPALInputPaneInteraction** ppInputPaneInteraction)
    {
        UNREFERENCED_PARAMETER(pInputPaneHandler);
        UNREFERENCED_PARAMETER(ppInputPaneInteraction);
        return E_NOTIMPL;
    }

    // IPALAutomationServices
    virtual _Check_return_ HRESULT GetAutomationProvider(_In_ IXcpHostSite *pSite, _In_ CDependencyObject *pElement, _Outptr_ IUnknown** ppProvider)
    {
        UNREFERENCED_PARAMETER(pSite);
        UNREFERENCED_PARAMETER(pElement);
        UNREFERENCED_PARAMETER(ppProvider);
        return E_NOTIMPL;
    }

    // IPALThemingServices
    virtual _Check_return_ HRESULT GetFadeInThemeAnimationData(_Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms)
    {
        UNREFERENCED_PARAMETER(pCount);
        UNREFERENCED_PARAMETER(ppTransforms);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT GetFadeOutThemeAnimationData(_Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms)
    {
        UNREFERENCED_PARAMETER(pCount);
        UNREFERENCED_PARAMETER(ppTransforms);
        return E_NOTIMPL;
    }

    // IPALPrintingServices

    virtual _Check_return_ HRESULT CreateD2DPrintFactoryAndTarget(_Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory, _Outptr_ IPALPrintTarget** ppPALPrintTarget)
    {
        UNREFERENCED_PARAMETER(ppPrintFactory);
        UNREFERENCED_PARAMETER(ppPALPrintTarget);
        return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT CreateD2DPrintingData(_Outptr_ IPALD2DPrintingData** ppPrintingData)
    {
        UNREFERENCED_PARAMETER(ppPrintingData);
        return E_NOTIMPL;
    }

    // IPALProcessCharacteristics
    virtual bool IsProcessPackaged()
    {
        VERIFY_FAIL();
        return false;
    }

};

void CreateMockPlatformServices(IMockPlatformServices ** services)
{
    *services = new MockPlatformServices();
}

