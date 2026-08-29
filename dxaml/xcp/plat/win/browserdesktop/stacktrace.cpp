// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Walks the stack and returns file name and line number information
//      for the top 4 methods.

#include "precomp.h"

#if XCP_MONITOR

// On x86 Windows use the debug help library to obtain stack trace.
//
// The checked memory code in core\debug\memory.cpp uses this to
// record the call stack for each allocation, in order to help
// diagnose leaks.


static XINT32 g_fSymbolsInitialized = FALSE;
static HANDLE g_hProcess = 0;




//------------------------------------------------------------------------
//
//  Method:  CaptureStack
//
//      Walks the stack and records the return address for each stack
//      frame for up to cMaxCallers frames.
//
//------------------------------------------------------------------------
typedef USHORT (*CAPTURESTACKBACKTRACEFUNCTION)(ULONG, ULONG, PVOID *, PULONG);

_Check_return_ HRESULT CWindowsServices::CaptureStack(
    _In_                            XUINT32    cMaxCallers,
    _Out_                           XUINT32   *pcCallers,
    _Outptr_result_buffer_(cMaxCallers) XUINT64  **ppCallers,
    _In_                            XUINT32    cIgnoreLevels
)
{
    ULONG_PTR *pCallers = NULL;
    XUINT64* p64Callers = NULL;

    static XINT32 fInit = FALSE;
    static CAPTURESTACKBACKTRACEFUNCTION fnCaptureStackBackTrace = NULL;

    if (!fInit)
    {
        HMODULE module = GetModuleHandleA("ntdll.dll");

        fnCaptureStackBackTrace = (CAPTURESTACKBACKTRACEFUNCTION)
            GetProcAddress(module, "RtlCaptureStackBackTrace");

        fInit = TRUE;
    }

    *pcCallers = 0;
    *ppCallers = NULL;

    if (fnCaptureStackBackTrace)
    {
        pCallers = (ULONG_PTR*) HeapAlloc( GetProcessHeap(), 0, (sizeof(pCallers[0]) * cMaxCallers) );
        p64Callers = (XUINT64*) HeapAlloc( GetProcessHeap(), 0, (sizeof(p64Callers[0]) * cMaxCallers) );

        memset(pCallers, 0, sizeof(pCallers[0]) * cMaxCallers);
        memset(p64Callers, 0, sizeof(p64Callers[0]) * cMaxCallers);

        // Lets stash away the entire stack trace if we have a
        // backtrace function.

        ULONG hash;
        USHORT frames = fnCaptureStackBackTrace(
            (ULONG) cIgnoreLevels,
            cMaxCallers,
            (PVOID*) pCallers,
            &hash
            );

        *pcCallers = frames;

        while (frames--)
        {
            p64Callers[frames] = pCallers[frames];
        }

        *ppCallers = p64Callers;

        p64Callers = NULL;
    }

    if ( p64Callers )
    {
        HeapFree( GetProcessHeap(), 0, p64Callers );
    }

    if ( pCallers )
    {
        HeapFree( GetProcessHeap(), 0, pCallers );
    }
    RRETURN(S_OK); //RRETURN_REMOVAL
}




//------------------------------------------------------------------------
//
//  Method:  GetCallerSourceLocations
//
//      Looks up the file name and line number for each return address
//      recorder by CaptureStack above.
//
//------------------------------------------------------------------------

void LoadSymbolsForModule(char *pModuleName)
{
    HMODULE    moduleHandle;
    MODULEINFO moduleInfo;
    DWORD64    loadAddress;
    DWORD      lastError;

    moduleHandle = GetModuleHandleA(pModuleName);
    lastError = GetLastError();

    if (moduleHandle)
    {
        if (GetModuleInformation(g_hProcess, moduleHandle, &moduleInfo, sizeof(moduleInfo)))
        {
            loadAddress = SymLoadModuleEx(
                g_hProcess,   // hProcess
                NULL,         // hFile
                pModuleName,  // ImageName
                NULL,         // ModuleName (shortcut name)
                (DWORD64) moduleInfo.lpBaseOfDll,
                (DWORD)   moduleInfo.SizeOfImage,
                NULL,         // MODLOAD_DATA
                0             // flags
            );
        }
        lastError = GetLastError();
    }
}

typedef BOOL (WINAPI *PSYM_FROM_ADDR)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);

_Check_return_ HRESULT CWindowsServices::GetCallerSourceLocations(
    _In_                         XUINT32          cCallers,
    _In_reads_(cCallers)        XUINT64         *pCallers,
    _Outptr_result_buffer_(cCallers) ICallingMethod **ppCallingMethods  // Receives pointer to array of cCallers ICallingMethods
)
{
    HRESULT          hr = S_OK;
    DWORD            characterOffset;
    IMAGEHLP_LINE64  line;
    BOOL             ok;
    XUINT32          iCaller;
    char            *pFilename;  // result from dbghelp.dll is ANSI
    XUINT32          i;
    CCallingMethod  *pCallingMethods = NULL;
    SYMBOL_INFO     *pSymbol = NULL;
    const XUINT32    MaxNameLength = 256;  // Size ogf string buffer for receiving symbol info

    static XINT32 fLoadedDbgHelp = FALSE;
    static PSYM_FROM_ADDR pfnSymFromAddr = NULL;

    // Delayload DbgHelp SymFromAddr API so that we don't rely on having DbgHelp version on Win2K
    // SymFromAddr is supported on DbgHelp.dll 5.1 or later version.
    if (!fLoadedDbgHelp)
    {
        HMODULE hModuleDbgHelp = LoadLibraryExW(L"dbghelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hModuleDbgHelp)
        {
            pfnSymFromAddr = (PSYM_FROM_ADDR) GetProcAddress(hModuleDbgHelp, "SymFromAddr");
        }
        fLoadedDbgHelp = TRUE;
    }
    IFCPTR(pfnSymFromAddr);

    // Start by creating the result and setting it empty

    pCallingMethods = new CCallingMethod[cCallers];
    memset(pCallingMethods, 0, sizeof(pCallingMethods[0]) * cCallers);

    *ppCallingMethods = NULL;

    // Initialize symbol handling if required

    if (!g_fSymbolsInitialized)
    {
        SymSetOptions(
            SYMOPT_LOAD_LINES
          | SYMOPT_DEFERRED_LOADS
        //| SYMOPT_IGNORE_CVREC
        //| SYMOPT_IGNORE_IMAGEDIR
        //| SYMOPT_LOAD_ANYTHING
          | SYMOPT_UNDNAME
          | SYMOPT_DEBUG
        );

        if (!g_hProcess)
        {
            g_hProcess = GetCurrentProcess();
        }

        // Mock10 will call SymInitialize as well. We preemptively call
        // SymCleanup here to release any resources it still holds. As we're
        // in the final cleanup of the DLL this should be acceptable.
        SymCleanup(g_hProcess);
        IFCW32(SymInitialize(g_hProcess, NULL, TRUE));
        g_fSymbolsInitialized = TRUE;
    }

    // Allocate buffer for receiving symbol information

    pSymbol = (SYMBOL_INFO*) new XUINT8[sizeof(SYMBOL_INFO) + MaxNameLength];


    for (iCaller=0; iCaller<cCallers; iCaller++)
    {
        // Determine file name and line of caller

        line.SizeOfStruct = sizeof(line);
        ok = SymGetLineFromAddr64(
            g_hProcess,
            pCallers[iCaller],
           &characterOffset,
           &line
        );

        if (ok)
        {
            XUINT32 filenamePosition = 0;
            // Extract file name and line number to caller information

            pCallingMethods[iCaller].iLine = line.LineNumber;

            // Find filename without path.

            pFilename = line.FileName;
            filenamePosition = 0;
            while (*pFilename && (pFilename < line.FileName+1000))
            {
                if (    *pFilename == '\\'
                    ||  *pFilename == '/')
                {
                    filenamePosition = XUINT32(pFilename - line.FileName) + 1;
                }
                pFilename++;
            }
            pFilename = line.FileName + filenamePosition;
            if (*pFilename == 0)
            {
                // There were no separators in the filename
                pFilename = line.FileName;
            }

            for (i=0; i<(ARRAY_SIZE(pCallingMethods[iCaller].szFilename)-1) && pFilename[i]; i++)
            {
                pCallingMethods[iCaller].szFilename[i] = pFilename[i];
            }
            pCallingMethods[iCaller].szFilename[i] = 0;
        }
        else
        {
            pCallingMethods[iCaller].szFilename[0] = 0; // Couldn't get symbol data for this address.
            pCallingMethods[iCaller].iLine = 0;
        }

        // Determine function name

        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen   = MaxNameLength;
        ok = pfnSymFromAddr(
            g_hProcess,
            pCallers[iCaller],
            0,
            pSymbol
        );

        if (ok)
        {
            // Extract symbol name, converting to Unicode
            for (i=0; i<pSymbol->NameLen && i<ARRAY_SIZE(pCallingMethods[iCaller].szSymbolName); i++)
            {
                pCallingMethods[iCaller].szSymbolName[i] = pSymbol->Name[i];
            }
            // Add zero terminator
            if (pSymbol->NameLen < ARRAY_SIZE(pCallingMethods[iCaller].szSymbolName))
            {
                pCallingMethods[iCaller].szSymbolName[pSymbol->NameLen] = 0;
            }
            else
            {
                pCallingMethods[iCaller].szSymbolName[ARRAY_SIZE(pCallingMethods[iCaller].szSymbolName)-1] = 0;
            }
        }
    }

    *ppCallingMethods = pCallingMethods;
    pCallingMethods = NULL;

Cleanup:
    delete [] pCallingMethods;
    delete [] (XUINT8*)pSymbol;
    return S_OK; // Note, we never fail, we just sometimes return no records.
}


void CWindowsServices::FreeCallingMethods(ICallingMethod *pCallers)
{
    CCallingMethod *pCallingMethods = (CCallingMethod*)pCallers;
    delete [] pCallingMethods;
}

#endif


