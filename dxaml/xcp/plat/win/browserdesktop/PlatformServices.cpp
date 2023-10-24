// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <commctrl.h>

// Enable graphics device manager creation
#include "windowsgraphicsdevicemanager.h"

#include <DWrite.h>
#include <DWrite_2.h>
#include "DWriteTypes.h"
#include "DWriteFontAndScriptServices.h"

#include <microsoft.ui.input.h>

#include "TextSurrogates.h"

// Enable a bunch of host features (should be in host)

#include "hal.h"
#include "host.h"
#include "framecounter.h"
#include "corep.h"
#include "commonbrowserhost.h"
#include "asyncdownloadrequestmanager.h"

#include "winbrowserhost.h"

#include "d3d11.h"

#include <XamlBehaviorMode.h>

#include "DXamlServices.h"

extern XHANDLE ghHeap;
extern HINSTANCE g_hInstance;

#define SAFE_GLOBALFREE(p)  if( NULL != p ) { ::GlobalFree( p ); p = NULL; }

extern _Check_return_ HRESULT NullTerminateString(_In_ XUINT32 cString, _In_reads_(cString) const WCHAR *pString, _Outptr_result_z_ wchar_t **ppwsz);

//------------------------------------------------------------------------
//
//  Function:   InitCPUCores
//
//  Synopsis:
//      Queries the number of CPU cores
//
//------------------------------------------------------------------------
XUINT32
ComputeCPUCoreCount()
{
    HANDLE          hCurrentProcessHandle;
    DWORD_PTR       dwProcessAffinity, dwSystemAffinity;
    XUINT32         cCPUCores = 1;

    // The operating system may limit the processors that this process may run on.  It happens
    // when the bitmasks dwProcessAffinity and dwSystemAffinity are different.

    hCurrentProcessHandle = GetCurrentProcess();
    if (hCurrentProcessHandle)
    {
        if (GetProcessAffinityMask(hCurrentProcessHandle, &dwProcessAffinity, &dwSystemAffinity))
        {
            cCPUCores = 0;

            // Count bits to count cores
            while (dwProcessAffinity)
            {
                dwProcessAffinity &= (dwProcessAffinity - 1); // consume a bit
                cCPUCores++;
            }

            cCPUCores = MAX(1, cCPUCores); // ensure we report at least one core
        }
    }

    return cCPUCores;
}

CWindowsServices::CWindowsServices()
{
    m_perfMarkerType = PerfMarkersDisabled;
    m_perfCheckCompleted = FALSE;

#if XCP_MONITOR && DBG
    m_pTraceSink = NULL;
#endif
    m_pRenderCounters = NULL;
    m_eFrequency = 0.0;

    m_isDirectManipulationSupported = FALSE;
    m_hasCheckedForDirectManipulationSupport = FALSE;
}

CWindowsServices::~CWindowsServices()
{
    delete m_pRenderCounters;
    CleanupDebugTrace();
}

//------------------------------------------------------------------------
//
//  Function:   CWindowsServices::GetMemoryCount
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
XUINT64 CWindowsServices::GetMemoryCount()
{
    XUINT64 cResult = 0;

    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;

    pmc.WorkingSetSize = 0;

    if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
    {
        cResult = (XUINT64) pmc.WorkingSetSize;
    }

    return cResult;
}

//------------------------------------------------------------------------
//
//  Method:   FileCreate
//
//  Synopsis:
//      Opens an existing file for read access and returns a token that can be
//  used in other file services methods.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWindowsServices::FileCreate(
    _In_ XUINT32 cFileName,
    _In_reads_(cFileName) const WCHAR *pFileName,
    _In_ XINT32 fileOptions,
    _Outptr_ IPALFile **ppFile
)
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

//------------------------------------------------------------------------
//
//  Method:   ProcessGetPriority
//
//  Synopsis:
//      Returns the priority class for the current process.
//
//------------------------------------------------------------------------

XINT32
CWindowsServices::ProcessGetPriority()
{
    XINT32  nPriority;

    nPriority = ::GetPriorityClass(GetCurrentProcess());

// Convert the OS priority to a PAL priority

    switch (nPriority)
    {
    case IDLE_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_IDLE;
        break;

    case BELOW_NORMAL_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_LOWERED;
        break;

    case NORMAL_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_NORMAL;
        break;

    case ABOVE_NORMAL_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_ELEVATED;
        break;

    case HIGH_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_HIGH;
        break;

    case REALTIME_PRIORITY_CLASS:
        nPriority = PAL_PROCESS_PRIORITY_REAL_TIME;
        break;

    default:
        nPriority = PAL_PROCESS_PRIORITY_NORMAL;
        break;
    }

    return nPriority;
}

//------------------------------------------------------------------------
//
//  Method:   ProcessSetPriority
//
//  Synopsis:
//      Attempts to set the priority class of the current process.
//
//------------------------------------------------------------------------

HRESULT
CWindowsServices::ProcessSetPriority(_In_ XINT32 nPriority)
{
// Convert the PAL priority to an OS priority

    switch (nPriority)
    {
    case PAL_PROCESS_PRIORITY_IDLE:
        nPriority = IDLE_PRIORITY_CLASS;
        break;

    case PAL_PROCESS_PRIORITY_LOWERED:
        nPriority = BELOW_NORMAL_PRIORITY_CLASS;
        break;

    case PAL_PROCESS_PRIORITY_NORMAL:
        nPriority = NORMAL_PRIORITY_CLASS;
        break;

    case PAL_PROCESS_PRIORITY_ELEVATED:
        nPriority = ABOVE_NORMAL_PRIORITY_CLASS;
        break;

    case PAL_PROCESS_PRIORITY_HIGH:
        nPriority = HIGH_PRIORITY_CLASS;
        break;

    case PAL_PROCESS_PRIORITY_REAL_TIME:
        nPriority = REALTIME_PRIORITY_CLASS;
        break;

    default:
        break;
    }

    return (::SetPriorityClass(GetCurrentProcess(), nPriority)) ? S_OK : E_FAIL;
}

//------------------------------------------------------------------------
//
//  Method:   IsSupportedPlatform
//
//  Synopsis:
//      Returns TRUE if the platform is supported.
//
//------------------------------------------------------------------------

_Check_return_ bool
CWindowsServices::IsSupportedPlatform()
{
#if defined(_ARM_) || defined(_ARM64_)
    return true;
#else
    // On x86 and amd64 platforms, check to be sure that SSE1 is supported
    return IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) ? true : false;
#endif
}

//------------------------------------------------------------------------
//
//  Method:   GetKeyboardModifiersState
//
//  Synopsis:
//      Get the current states of modifiers keys on Windows keyboard.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
    CWindowsServices::GetKeyboardModifiersState( _Out_ XUINT32 *pModifiers)
{
    XUINT32 uModifiers = 0;

    if (!pModifiers)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (!m_keyboardInputStatics)
    {
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputKeyboardSource).Get(),
            &m_keyboardInputStatics));
    }

    wuc::CoreVirtualKeyStates keyState;

    IFC_RETURN(m_keyboardInputStatics->GetKeyStateForCurrentThread(wsy::VirtualKey::VirtualKey_Menu, &keyState));
    if (keyState & wuc::CoreVirtualKeyStates_Down)
    {
        uModifiers |= KEY_MODIFIER_ALT;  // Alt key.
    }

    IFC_RETURN(m_keyboardInputStatics->GetKeyStateForCurrentThread(wsy::VirtualKey::VirtualKey_Control, &keyState));
    if (keyState & wuc::CoreVirtualKeyStates_Down)
    {
        uModifiers |= KEY_MODIFIER_CTRL;  // Ctrl key.
    }

    IFC_RETURN(m_keyboardInputStatics->GetKeyStateForCurrentThread(wsy::VirtualKey::VirtualKey_Shift, &keyState));
    if (keyState & wuc::CoreVirtualKeyStates_Down)
    {
        uModifiers |= KEY_MODIFIER_SHIFT;  // Shift key.
    }

    IFC_RETURN(m_keyboardInputStatics->GetKeyStateForCurrentThread(wsy::VirtualKey::VirtualKey_LeftWindows, &keyState));
    if (keyState & wuc::CoreVirtualKeyStates_Down)
    {
        uModifiers |= KEY_MODIFIER_WINDOWS;  // Windows key.
    }

    IFC_RETURN(m_keyboardInputStatics->GetKeyStateForCurrentThread(wsy::VirtualKey::VirtualKey_RightWindows, &keyState));
    if (keyState & wuc::CoreVirtualKeyStates_Down)
    {
        uModifiers |= KEY_MODIFIER_WINDOWS;  // Windows key.
    }

    *pModifiers = uModifiers;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsOnRemoteDesktopSession
//
//  Synopsis:
//      Detect if the current session is a remote desktop session.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::IsOnRemoteDesktopSession( _Out_ bool* pfOnRemoteDesktop )
{
    HRESULT hr = S_OK;

    IFCPTR(pfOnRemoteDesktop);

    *pfOnRemoteDesktop = GetSystemMetrics(SM_REMOTESESSION) || GetSystemMetrics(SM_REMOTECONTROL);

Cleanup:
    RRETURN(hr);
}

#if XCP_MONITOR
//------------------------------------------------------------------------
//
//  Method:  CWindowsServices::GetMonitorBuffer
//
//  Synopsis:
//
//      Finds and returns address of monitoring buffer shared with xcpmon process.
//
//------------------------------------------------------------------------

HRESULT CWindowsServices::GetMonitorBuffer(_Out_ MonitorBuffer **ppMonitorBuffer)
{
    HRESULT        hr             = S_OK;
    MonitorBuffer *pMonitorBuffer = NULL;

    SetLastError(0);

    HANDLE hMapFile = OpenFileMapping(
                                      FILE_MAP_ALL_ACCESS,   // read/write access
                                      FALSE,                 // do not inherit the name
                                      L"XcpMonitor"          // XcpMonitor's shared memory
                                      );

    if (!hMapFile)
    {
        // Monitor is not active. No debug tracing.
        hr = S_OK; // It's not actually an error for there to be no monitor
        goto Cleanup;
    }

    // Map debug globals structure to the start of the shared memory block

    IFCW32(pMonitorBuffer = (MonitorBuffer*)MapViewOfFile(
                                                          hMapFile,
                                                          FILE_MAP_ALL_ACCESS,  // read/write permission
                                                          0,
                                                          0,
                                                          sizeof(MonitorBuffer)
                                                          ));

    // Lock the buffer

    if (PAL_InterlockedIncrement((XINT32*)&pMonitorBuffer->m_bInUse) > 1)
    {
        UnmapViewOfFile(pMonitorBuffer);
        CloseHandle(hMapFile);

        // Buffer is already in use. Don't use it.
        IFC(E_FAIL);
    }

    // Record our process Id to help xcpmon tracking whether we are still active
    pMonitorBuffer->m_processId = GetCurrentProcessId();

    // Record file mapping handle so we can clear up later
    pMonitorBuffer->m_fileHandle = (XUINT64)(INT_PTR)hMapFile;

    *ppMonitorBuffer = pMonitorBuffer;

 Cleanup:
    return hr;
}
#endif

_Check_return_ HRESULT
CWindowsServices::XcpRenderCountersInitialize()
{
    RRETURN(CWinRenderCounters::Create(&m_pRenderCounters));
}

_Check_return_ HRESULT
CWindowsServices::XcpRenderCountersIncrement(_In_ XUINT32 uCounter)
{
    if (m_pRenderCounters)
    {
        RRETURN(m_pRenderCounters->Increment(uCounter));
    }
    RRETURN(S_OK);
}

HRESULT
CWinRenderCounters::Create(_Out_ IXcpRenderCounters **ppRenderCounters)
{
    HRESULT hr = S_OK;

    CWinRenderCounters* counters = new CWinRenderCounters();

#if XCP_TRACE
    // This variable is freed after the leak trace system is torn down. It is not leaked.
    ::XcpDebugSetLeakDetectionFlag(counters, true);
#endif

    *ppRenderCounters = NULL;

    counters->m_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS,   // read/write access
                                           FALSE,                 // do not inherit the name
                                           L"XAMLRenderCounters"    // XAMLRenderCounters's shared memory
                                           );

    if (!counters->m_hMapFile) // no one is listening, so just go to Cleanup
    {
        goto Cleanup;
    }

    IFCW32(counters->m_pCounters = (RenderCounters*)MapViewOfFile(counters->m_hMapFile,
                                                                   FILE_MAP_ALL_ACCESS,  // read/write permission
                                                                   0,
                                                                   0,
                                                                   sizeof(RenderCounters)
                                                                   ));
    ZeroMemory(counters->m_pCounters, sizeof(RenderCounters));

    *ppRenderCounters = counters;
    counters = NULL;
Cleanup:
    delete counters;
    RRETURN(hr);
}

HRESULT
CWinRenderCounters::Increment(_In_ XUINT32 uCounter)
{
    switch(uCounter)
    {
        case XcpRenderCountersGenerateEdges:
            m_pCounters->uGenerateEdges++;
            break;
    }
    RRETURN(S_OK);
}

CWinRenderCounters::~CWinRenderCounters()
{
    if (m_pCounters)
    {
        UnmapViewOfFile(m_pCounters);
        m_pCounters = NULL;
    }
    if (m_hMapFile)
    {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }
}

_Check_return_ HRESULT CWindowsServices::CreateGraphicsDeviceManager(_Outptr_ WindowsGraphicsDeviceManager **ppGraphicsDeviceManager)
{
    HRESULT hr = S_OK;

    WindowsGraphicsDeviceManager *pGraphicsDeviceManager = NULL;

    IFC(WindowsGraphicsDeviceManager::Create(&pGraphicsDeviceManager));

    *ppGraphicsDeviceManager = pGraphicsDeviceManager;
    pGraphicsDeviceManager = NULL; // pass ownership of the object to the caller

Cleanup:
    ReleaseInterfaceNoNULL(pGraphicsDeviceManager);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   PrintPage
//
//  Synopsis: Used by the printing feature to print a single page at a
//      time.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CWindowsServices::PrintPage(
                            _In_ void* pvBitmap,
                            _In_ IPALPrintingData* pPalPD)
{
    HRESULT hr = S_OK;
    XINT32  nPrintingError = 0;
    BITMAPINFO bitmapinfo;
    CWinPrintingData *pWinPD = NULL;

    IFCPTR(pPalPD);
    pWinPD = static_cast<CWinPrintingData*>(pPalPD);
    IFCEXPECT_ASSERT(pWinPD);

    IFCEXPECT_ASSERT(pWinPD->GetPrinterHandle());

    nPrintingError = StartPage(pWinPD->GetPrinterHandle());
    if(nPrintingError <= 0)
    {
        IFC(E_FAIL);
    }

    ZeroMemory(&bitmapinfo, sizeof(bitmapinfo));
    bitmapinfo.bmiHeader.biSize = sizeof(bitmapinfo.bmiHeader);
    bitmapinfo.bmiHeader.biWidth = pWinPD->GetPrintableAreaWidth();
    bitmapinfo.bmiHeader.biHeight = pWinPD->GetPrintableAreaHeight() * -1;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 32;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;

    if (FALSE == pWinPD->GetShouldStretch())
    {
        const int scanlinesSet = SetDIBitsToDevice(
            pWinPD->GetPrinterHandle(),
            0,
            0,
            bitmapinfo.bmiHeader.biWidth,
            bitmapinfo.bmiHeader.biHeight * -1,
            0,
            0,
            0,
            bitmapinfo.bmiHeader.biHeight * -1,
            pvBitmap,
            &bitmapinfo,
            DIB_RGB_COLORS);

        if (scanlinesSet != bitmapinfo.bmiHeader.biHeight * -1)
        {
            IFC(E_FAIL);
        }
    }
    else
    {
        const int scanlinesCopied = StretchDIBits(
            pWinPD->GetPrinterHandle(),
            0,
            0,
            pWinPD->GetPrinterWidthInPixels(),
            pWinPD->GetPrinterHeightInPixels(),
            0,
            0,
            bitmapinfo.bmiHeader.biWidth,
            bitmapinfo.bmiHeader.biHeight * -1,
            pvBitmap,
            &bitmapinfo,
            DIB_RGB_COLORS,
            SRCCOPY);

        if (scanlinesCopied != bitmapinfo.bmiHeader.biHeight * -1)
        {
            IFC(E_FAIL);
        }
    }

    nPrintingError = EndPage( pWinPD->GetPrinterHandle());

    if(nPrintingError <= 0)
    {
        hr = E_FAIL;
    }
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CWindowsServices::CreateFontAndScriptServices
//
//  Synopsis:
//     Provides platform's Font and Script Services.
//
//------------------------------------------------------------------------
HRESULT CWindowsServices::CreateFontAndScriptServices(_Outptr_ IPALFontAndScriptServices** ppFontAndScriptServices)
{
    HRESULT hr = S_OK;
    DWriteFontAndScriptServices *pDWriteFontAndScriptServices = NULL;

    IFCPTR(ppFontAndScriptServices);
    IFC(DWriteFontAndScriptServices::Create(&pDWriteFontAndScriptServices));

    *ppFontAndScriptServices = pDWriteFontAndScriptServices;
    pDWriteFontAndScriptServices = NULL;

Cleanup:
    ReleaseInterface(pDWriteFontAndScriptServices);
    RRETURN(hr);
}
