// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XcpAllocation.h"

#if XCP_MONITOR

#undef ASSERT
#define ASSERT(a) MONITORASSERT(a)

#undef MIN
#undef min
#include <algorithm>

// Global variables for monitoring

struct WriteableMonitorBuffer;
WriteableMonitorBuffer *g_pMonitorBuffer = NULL;
IPALClock              *g_pPalClock      = NULL;
XDOUBLE                 g_startTime      = 0.0;
XUINT32                 g_Section        = 0;             // Current section as identified by ENTERSECTION/LEAVESECTION


// Size of monitor/trace output formatting string buffer

#define MONITOR_STRING_BUFFER_LENGTH 400


// Length of header (filename, line number and task ID) used when calling
// OutputDebugString directly (i.e. when there's no monitor attached).

#define DEBUG_HEADER_STRING_LENGTH 30



//------------------------------------------------------------------------
//
//  Implementation of circular buffer write APIs
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Struct:   ReservedBufferWriter
//
//  Synopsis:
//
//      All writes to the circular buffer must be made through this structure:
//
//      The writer first reserves an area of the circular buffer then puts data
//      into that reserved area. The ReservedBufferWriter controls access to the
//      reserved buffer area.
//
//      Circular buffer reserved space can be in two blocks when the
//      space wraps around the end of the buffer.
//
//      The first 4 bytes are always the message type, which is set to 0 while
//      the buffer is being filled, and only set non-zero when
//      the rest of the buffer is complete.
//
//      See circular buffer docs below for more details.
//------------------------------------------------------------------------

struct ReservedBufferWriter
{
    // Reserved buffer writer - writes into space reserved in the
    // circular buffer.

private:
    ReservedBufferWriter() {}
    ~ReservedBufferWriter() {}


public:
    void    *m_pFirstBlock;
    XUINT32  m_firstBlockSize;
    void    *m_pSecondBlock;
    XUINT32  m_secondBlockSize;
    XUINT32  m_offset;

    //------------------------------------------------------------------------
    //
    //  Method:   ReservedBufferWriter constructor
    //
    //  Synopsis:
    //
    //      Creates an empty reserved buffer structure.
    //------------------------------------------------------------------------

    void Initialize()
    {
        m_pFirstBlock     = NULL;
        m_firstBlockSize  = 0;
        m_pSecondBlock    = NULL;
        m_secondBlockSize = 0;
        m_offset          = 0;
    }


    //------------------------------------------------------------------------
    //
    //  Method:   ReservedBufferWriter::SetType
    //
    //  Synopsis:
    //
    //      Sets the type into the header.
    //
    //      Must not be called until all other data has been set in the buffer
    //      as setting the type field indicates to the reader that the buffer
    //      is complete.
    //------------------------------------------------------------------------

    void SetType(XUINT32 type)
    {
        MONITORASSERT(m_offset == m_firstBlockSize + m_secondBlockSize);

        memcpy(m_pFirstBlock, &type, std::min(m_firstBlockSize, 4u));
        if (m_firstBlockSize < 4)
        {
            memcpy(m_pSecondBlock, ((XUINT8*)&type) + m_firstBlockSize, 4 - m_firstBlockSize);
        }

    }


    //------------------------------------------------------------------------
    //
    //  Method:   ReservedBufferWriter::WriteBytes
    //
    //  Synopsis:
    //
    //      Adds data to that already in the ReservedBufferWriter.
    //
    //      Data is added initially beyond the header.
    //
    //------------------------------------------------------------------------

    void WriteBytes(XUINT32 cSize, void *pSource)
    {
        MONITORASSERT(m_offset + cSize <= m_firstBlockSize + m_secondBlockSize);

        if (cSize) // (Caller may request 0 bytes written, in which case we do nothing)
        {
            XUINT32 sizeToWriteInFirstBlock = std::min(cSize, m_firstBlockSize - m_offset);

            if (m_offset < m_firstBlockSize)
            {
                memcpy(
                    ((XUINT8*)m_pFirstBlock) + m_offset,
                    pSource,
                    sizeToWriteInFirstBlock
                );
                m_offset += sizeToWriteInFirstBlock;
                pSource   = ((XUINT8*)pSource) + sizeToWriteInFirstBlock;
                cSize    -= sizeToWriteInFirstBlock;
            }

            if (cSize > 0)
            {
                memcpy(
                    ((XUINT8*)m_pSecondBlock) + m_offset - m_firstBlockSize,
                    pSource,
                    cSize
                );
                m_offset += cSize;
            }
        }
    }
};




//------------------------------------------------------------------------
//
//  Method:   NewReservedBufferWriter
//
//  Synopsis:
//
//      Allocates a ReservedBufferWriter directly from the heap, bypassing
//      memory allocation monitoring.
//
//------------------------------------------------------------------------

ReservedBufferWriter *NewReservedBufferWriter()
{
    ReservedBufferWriter *pWriter =
        (ReservedBufferWriter*) XcpAllocation::OSMemoryAllocateFailFast(sizeof(ReservedBufferWriter));

    if (pWriter)
    {
        pWriter->Initialize();
    }

    return pWriter;
}




//------------------------------------------------------------------------
//
//  Method:   DeleteReservedBufferWriter
//
//  Synopsis:
//
//      Deallocates a ReservedBufferWriter directly from the heap, bypassing
//      memory allocation monitoring.
//
//------------------------------------------------------------------------

void DeleteReservedBufferWriter(ReservedBufferWriter **ppWriter)
{
    if (ppWriter && *ppWriter)
    {
        XcpAllocation::OSMemoryFree(*ppWriter);
        *ppWriter = NULL;
    }
}



//------------------------------------------------------------------------
//
//  struct:  WriteableMonitorBuffer
//
//  Synopsis:
//
//      Adds methods for safely adding data to the circular buffer.
//
//------------------------------------------------------------------------

struct WriteableMonitorBuffer : public MonitorBuffer
{
    //------------------------------------------------------------------------
    //
    //  Method:   WriteableMonitorBuffer::LockWrites
    //
    //  Synopsis:
    //
    //      Uses InterlockedIncrement to synchronize access between multiple
    //      threads attempting to add data to the circular buffer at the
    //      same time.
    //
    //      Note that the code below is written so that the lock is held only
    //      while the next writablebuffer area is being allocated. The lock
    //      remains held for very short periods of time.
    //
    //------------------------------------------------------------------------

    void LockWrites()
    {
        XINT32 lockCount = PAL_InterlockedIncrement(&m_inLocked);
        while (lockCount > 1)
        {
            // Someone else got here first. Wait for them.
            PAL_InterlockedDecrement(&m_inLocked);
            GetPALDebuggingServices()->YieldSlice();
            lockCount = PAL_InterlockedIncrement(&m_inLocked);
        }
    }




    //------------------------------------------------------------------------
    //
    //  Method:   WriteableMonitorBuffer::ReleaseWrites
    //
    //  Synopsis:
    //
    //      Releases the monitor buffer write lock previously set by a call
    //      to LockWrites.
    //
    //------------------------------------------------------------------------

    void ReleaseWrites()
    {
        PAL_InterlockedDecrement(&m_inLocked);
    }


    //------------------------------------------------------------------------
    //
    //  Method:   WriteableMonitorBuffer::Reserve
    //
    //  Synopsis:
    //
    //      Reserves space for a message header plus an addtional amount of
    //      buffer space, and initializes the header.
    //
    //      This is the only code that uses the LockWrites/ReleaseWrites methods.
    //
    //      The message header processClocks field is filled according to the
    //      data in the current threads Timing structure. The client must have
    //      used TIMESUSPEND for this data to be correct.
    //
    //      If the buffer is full, waits for space to become available.
    //
    //------------------------------------------------------------------------

    ReservedBufferWriter *Reserve(XUINT32 cAdditionalSize, XUINT64 processClocks)
    {
        ReservedBufferWriter *pWriter = NULL;
        MonitorMessageHeader  header;
        XUINT32               cTotalSize = sizeof(header) + cAdditionalSize;

        LockWrites();

        // From here onwards we have exclusive access to m_in.

        while (cTotalSize > TotalAvailableIn())
        {
            // Not enough room, wait for room to become available.
            ReleaseWrites();
            GetPALDebuggingServices()->YieldSlice();
            LockWrites();
        }

        pWriter = NewReservedBufferWriter();

        if (pWriter != NULL)
        {
            pWriter->m_pFirstBlock    = pIn();
            pWriter->m_firstBlockSize = std::min(cTotalSize, ContiguousAvailableIn());

            // Clear first 4 bytes (type field) of header, at least as much as exisits in first block

            memset(pIn(), 0, std::min(4u, pWriter->m_firstBlockSize));

            if (cTotalSize == pWriter->m_firstBlockSize)
            {
                // Everything fits in one block

                pWriter->m_pSecondBlock    = NULL;
                pWriter->m_secondBlockSize = 0;

                // Update 'in'

                if (m_in + pWriter->m_firstBlockSize < sizeof(m_buffer))
                {
                    m_in = m_in + pWriter->m_firstBlockSize;
                }
                else
                {
                    m_in = 0;
                }
            }
            else
            {
                // Second block always starts at beginning of circular buffer.

                pWriter->m_pSecondBlock    = &m_buffer;
                pWriter->m_secondBlockSize = cTotalSize - pWriter->m_firstBlockSize;

                // Clear any remaining bytes of the first 4 (type field) of the header that were not in the first block

                if (pWriter->m_firstBlockSize < 4)
                {
                    memset(m_buffer, 0, 4 - pWriter->m_firstBlockSize);
                }

                // Update 'in'

               m_in = pWriter->m_secondBlockSize;
            }

            // Initialize the header

            header.type          = 0;  // Must be 0 now, will be set non-zero only when message is complete
            header.length        = cTotalSize;
            header.processClocks = processClocks;
            header.runTime       = g_pPalClock->GetAbsoluteTimeInSeconds() - g_startTime;
            header.threadID      = GetCurrentThreadId();

            pWriter->WriteBytes(sizeof(header), &header);
        }


        ReleaseWrites();
        return pWriter;
    }
};





//------------------------------------------------------------------------
//
//  Method:   MonitorInitialize
//
//  Synopsis:
//
//  Finds the monitoring app shared memory section if any and initializes
//  global monitoring structures.
//
//------------------------------------------------------------------------
#if USE_VIRTUAL_ALLOC
void InitCheckedMemoryChainLock(XUINT8 bIsUseVirtualOverrideSet);  // (In core/debug/memory.cpp)
#else
void InitCheckedMemoryChainLock();  // (In core/debug/memory.cpp)
#endif
    

#if USE_VIRTUAL_ALLOC
_Check_return_ HRESULT XcpTraceMonitorInitialize(XUINT8 bIsUseVirtualOverrideSet)
#else
_Check_return_ HRESULT XcpTraceMonitorInitialize()
#endif
{
    HRESULT hr = S_OK;

#if USE_VIRTUAL_ALLOC
    InitCheckedMemoryChainLock(bIsUseVirtualOverrideSet);
#else
    InitCheckedMemoryChainLock();
#endif

    GetPALDebuggingServices()->InitializeThreadMonitoring();

    g_pMonitorBuffer = NULL;
    GetPALDebuggingServices()->GetMonitorBuffer((MonitorBuffer**)&g_pMonitorBuffer);  // Will fail if xcpmon is not running, and that's fine.

    if (g_pMonitorBuffer)
    {
        GetPALDebuggingServices()->SetTraceFlags(&g_pMonitorBuffer->m_traceFlags); // Use trace control flags from shared memory
    }

    g_pPalClock = NULL;
    GetPALCoreServices()->CreateClock(&g_pPalClock);
    if (g_pPalClock)
    {
        g_startTime = g_pPalClock->GetAbsoluteTimeInSeconds();
    }


//Cleanup:
    return hr;
}




//------------------------------------------------------------------------
//
//  Method:   XcpTraceMonitorShutdown
//
//  Synopsis:
//
//  Release monitoring data structures and disconnects from xcpmon.
//
//------------------------------------------------------------------------
void DeleteCheckedMemoryChainLock();  // (In core/debug/memory.cpp)

void XcpTraceMonitorShutdown()
{
    DeleteCheckedMemoryChainLock();

    GetPALDebuggingServices()->DeleteThreadMonitoring();

    ReleaseInterface(g_pPalClock);

    GetPALDebuggingServices()->ReleaseMonitorBuffer(g_pMonitorBuffer);
    g_pMonitorBuffer = NULL;
}




//------------------------------------------------------------------------
//
//  Method:   FormatHResult
//
//  Synopsis:
//
//  Returns a descriptive text string for a few known HRESULTs.
//
//------------------------------------------------------------------------

const WCHAR *FormatHResult(_In_ HRESULT hr)
{
    switch (hr)
    {
    case S_OK:                  return L"OK";
    case E_NOTIMPL:             return L"Not implemented";
    case E_INVALIDARG:          return L"Invalid argument";
    case 0x80070002L:           return L"File not found";
    case E_OUTOFMEMORY:         return L"Out of memory";
    case E_UNEXPECTED:          return L"Unexpected";
    case E_ACCESSDENIED:        return L"Access denied";
    case E_ABORT:               return L"Abort";
    case 0x80070216:            return L"Arithmetic overflow";
    case E_FAIL:                return L"Fail";
    case E_HANDLE:              return L"Bad handle";
    case E_NOINTERFACE:         return L"No interface";
    case E_POINTER:             return L"Bad pointer";
    default:                    return L"";
    }
}



//------------------------------------------------------------------------
//
//  Method:  StringLength
//
//  Synopsis:
//
//      Simple string length measurement.
//
//------------------------------------------------------------------------

XUINT32 StringLength(
    const WCHAR   *pString,
    XUINT32  cStringMax // Maximum length of input string
)
{
    if (!pString)
    {
        return 0;
    }

    XUINT32 cString = 0;

    while (cString < cStringMax  &&  pString[cString])
    {
        cString++;
    }

    return cString;
}




//------------------------------------------------------------------------
//
//  Method:   SendTraceMessage
//
//  Synopsis:
//
//  Sends a trace packet to the monitor process containing:
//    o  Class of trace information
//    o  Filename and line number of source location requesting the memory
//    o  String containing source code macro parameter triggering this message
//    o  String containing optional arguments
//    o Optional arguments
//
//------------------------------------------------------------------------

void SendTraceMessage(
               XUINT32  uClass,      // Class of monitoring event, a.g. assertion, trace
    _In_opt_z_ const WCHAR   *pFileName,
               XINT32   iLine,
               XINT32   iValue,      // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR   *pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR   *pMessage,    // Message text
               XUINT64  processClocks
)
{

#define MaxString 200 // Maximum length of individual strings that we'll send to xcpmon.

    XUINT32 fileNameSize      = pFileName   ? (StringLength(pFileName,   MaxString) * sizeof(WCHAR)) : 0;
    XUINT32 testStringSize    = pTestString ? (StringLength(pTestString, MaxString) * sizeof(WCHAR)) : 0;
    XUINT32 messageStringSize = pMessage    ? (StringLength(pMessage,    MaxString) * sizeof(WCHAR)) : 0;

    ReservedBufferWriter *pWriter = g_pMonitorBuffer->Reserve(
          sizeof(XUINT32)  // iLine
        + sizeof(XUINT32)  // iValue
        + sizeof(XUINT32)  // filename size
        + sizeof(XUINT32)  // test string size
        + sizeof(XUINT32)  // message string size
        + fileNameSize
        + testStringSize
        + messageStringSize,
        processClocks
    );

    pWriter->WriteBytes(sizeof(iLine),             &iLine);
    pWriter->WriteBytes(sizeof(iValue),            &iValue);
    pWriter->WriteBytes(sizeof(fileNameSize),      &fileNameSize);
    pWriter->WriteBytes(sizeof(testStringSize),    &testStringSize);
    pWriter->WriteBytes(sizeof(messageStringSize), &messageStringSize);

    if (fileNameSize)      pWriter->WriteBytes(fileNameSize,      (void *)pFileName);
    if (testStringSize)    pWriter->WriteBytes(testStringSize,    (void *)pTestString);
    if (messageStringSize) pWriter->WriteBytes(messageStringSize, (void *)pMessage);

    pWriter->SetType(uClass);  // Mark record as ready for consumption
    DeleteReservedBufferWriter(&pWriter);
}






//------------------------------------------------------------------------
//
//  Method:   SendMemoryMessage
//
//  Synopsis:
//
//  Sends a memory allocation/deallocation packet to the monitor process 
//  containing:
//    o  Class MonitorAllocationPerf or MonitorDeallocationPerf
//    o  Memory address
//    o  Size in bytes
//
//------------------------------------------------------------------------

void SendXcpMonMemoryMessage(
    XUINT32  uClass,      // MonitorAllocationPerf or MonitorDeallocationPerf
    XUINT64  memoryAddress,
    XUINT64  cBytes
)
{
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks

    ReservedBufferWriter *pWriter = g_pMonitorBuffer->Reserve(
          sizeof(XUINT64)  // memoryAddress
        + sizeof(XUINT64), // cBytes 
        pThreadMonitor->m_clocksAtLastSuspend
    );

    pWriter->WriteBytes(sizeof(XUINT64), &memoryAddress);
    pWriter->WriteBytes(sizeof(XUINT64), &cBytes);

    pWriter->SetType(uClass);  // Mark record as ready for consumption
    DeleteReservedBufferWriter(&pWriter);
    pThreadMonitor->Resume();
}

//------------------------------------------------------------------------
//
//  Method:  XcpStringCchPrintf
//
//  Synopsis:
//
//      Safe string formatting code. Collects vargs parameters and forwards
//  to GetPALThreadingServices()->StringCchVPrintf.
//
//------------------------------------------------------------------------
HRESULT XcpStringCchPrintf(
    WCHAR   *pString,
    XUINT32  cString,
    const WCHAR   *pFormat,
    ...
)
{
    va_list vargs;
    va_start(vargs, pFormat);
#ifndef __LP64__
    return GetPALDebuggingServices()->StringCchVPrintf(pString, cString, pFormat, vargs);
#else
    //FIX64.  It's not OK, really it isn't, but we need to deal with varargs holistically.
    return S_OK;
#endif
}

//------------------------------------------------------------------------
//
//  Method:  StringRightChar
//
//  Synopsis:
//
//      Return offset of rightmost occurrence of character in string
//
//------------------------------------------------------------------------

XUINT32 StringRightChar(
    const WCHAR   *pString,
    XUINT32  cStringMax, // Maximum length of input string
    WCHAR    chSearch    // Character to search for
)
{
    if (!pString)
    {
        return 0;
    }

    XUINT32 cString = 0;
    XUINT32 charPosition = 0;

    while (cString < cStringMax  &&  pString[cString])
    {
        if (pString[cString] == chSearch)
        {
            charPosition = cString;
        }
        cString++;
    }

    return charPosition;
}




//------------------------------------------------------------------------
//
//  Method:  LocateFileName
//
//  Synopsis:
//
//      Returns a pointer to the filename of a path+filename
//
//------------------------------------------------------------------------

const WCHAR *LocateFilename(_In_z_ const WCHAR *pFullFilename)
{
    XUINT32 offsetFileName = 0;
    offsetFileName = StringRightChar(pFullFilename, MaxString, L'\\');
    if (!offsetFileName)
    {
        offsetFileName = StringRightChar(pFullFilename, MaxString, L'/');
    }
    if (    (pFullFilename[offsetFileName] == L'\\')
        ||  (pFullFilename[offsetFileName] == L'/'))
    {
        offsetFileName++;
    }
    return pFullFilename + offsetFileName;
}




const WCHAR *GetMessageClassName(XUINT32 uClass)
{
    switch (uClass)
    {
    case MonitorAssert:                   return L"ASSERT";              break;
    case MonitorAssertSuccess:            return L"ASSERTSUCCESS";       break;
    case MonitorTrace:                    return L"TRACE";               break;
    case MonitorIfc:                      return L"IFC";                 break;
    case MonitorIfcOom:                   return L"IFCOOM";              break;
    case MonitorIfcPtr:                   return L"IFCPTR";              break;
    case MonitorIfcHndl:                  return L"IFCHNDL";             break;
    case MonitorIfcExpect:                return L"IFCEXPECT";           break;
    case MonitorIfcW32:                   return L"IFCW32";              break;
    case MonitorIfcOsx:                   return L"IFCOSX";              break;
    case MonitorEnterSection:             return L"ENTERSECTION";        break;
    case MonitorLeaveSection:             return L"LEAVESECTION";        break;
    case MonitorEnterMethod:              return L"ENTERMETHOD";         break;
    case MonitorLeaveMethod:              return L"LEAVEMETHOD";         break;
    case MonitorAllocation:               return L"Allocation";          break;
    case MonitorDeallocation:             return L"Deallocation";        break;
    case MonitorMemoryHeaderCorruption:   return L"Mem hdr corruption";  break;
    case MonitorMemoryTrailerCorruption:  return L"Mem tlr corruption";  break;
    }

    return L"Unknown event";
}



#if DBG


//------------------------------------------------------------------------
//
//  Method:   DisplayDebugMessage
//
//  Synopsis:
//
//      Used when not connected to xcpmon, formats the message and sends it
//  to the OS debug message mechanism.
//
//------------------------------------------------------------------------

void DisplayDebugMessage
(
               XUINT32  uClass,      // Class of monitoring event, a.g. assertion, trace
    _In_opt_z_ const WCHAR   *pFileName,
               XINT32   iLine,
               XINT32   iValue,      // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR   *pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR   *pMessage,    // Message text
               XUINT64  processClocks
)
{
    WCHAR  sourcePosition[40]; // Fixed length buffer for filename + line number
    WCHAR  message[400];       // Fixed length message formatting buffer


    // Skip displaying allocation and section messages unless required

    switch (uClass)
    {
    case MonitorAllocation:
    case MonitorDeallocation:
        if (!(GetPALDebuggingServices()->GetTraceFlags() & TraceAlloc))
        {
            return;
        }
        break;
    default: break;
    }

    // Format the source position as the filename (without path) plus the line number

    if ( uClass == MonitorRaw )
    {
        // Raw format - no file, line #, etc
        XcpStringCchPrintf(
            message, ARRAY_SIZE(message)-1,
            L"%s",
            pMessage
        );
    }
    else
    {
        XcpStringCchPrintf(
            sourcePosition,
            ARRAY_SIZE(sourcePosition)-1,
            L"%s[%d]",
            LocateFilename(pFileName),
            iLine
        );
        sourcePosition[ARRAY_SIZE(sourcePosition)-1] = 0; // Guarantee termination

        // Format the overall message
        XcpStringCchPrintf(
            message, ARRAY_SIZE(message)-1,
            L"%s %s: Source '%s', Value %d, Message %s",
            sourcePosition,
            GetMessageClassName(uClass),
            pTestString,
            iValue,
            pMessage
        );
    }

    message[ARRAY_SIZE(message)-1] = 0; // Guarantee termination

    // Issue the message
    GetPALDebuggingServices()->DebugOutputSz(message);
}


//------------------------------------------------------------------------
//
//  Method:   DisplayDebugDialog
//
//  Synopsis:
//
//      Formats the trace information for a debug assert message and
//  calls DebugAssertMessageBox.
//
//------------------------------------------------------------------------


bool DisplayDebugDialog(
               XUINT32  uClass,      // Class of monitoring event, a.g. assertion, trace
    _In_opt_z_ const WCHAR   *pFileName,
               XINT32   iLine,
               XINT32   iValue,      // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR   *pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR   *pMessage     // Message text
)
{
    WCHAR  message[400];       // Fixed length message formatting buffer

    // Format the overall message

    message[0] = 0;

    switch (uClass)
    {
    case MonitorAssert:
    case MonitorAssertSuccess:
        XcpStringCchPrintf(
            message, ARRAY_SIZE(message)-1,
            L"%s(%s): %s",
            GetMessageClassName(uClass),
            pTestString,
            pMessage
        );
        break;

    case MonitorMemoryHeaderCorruption:
    case MonitorMemoryTrailerCorruption:
        XcpStringCchPrintf(
            message, ARRAY_SIZE(message)-1,
            L"%s at %x: %s",
            GetMessageClassName(uClass),
            iValue,
            pTestString
        );
        break;
    }


    message[ARRAY_SIZE(message)-1] = 0; // Guarantee termination

    // return value is whether the caller should break in.
    return GetPALDebuggingServices()->DebugAssertMessageBox(message, pFileName, iLine);
}



#endif // #if DBG



//------------------------------------------------------------------------
//
//  Method:   XcpVTrace
//
//  Synopsis:
//
//   When connected to xcpmon, sends a trace message to xcpmon, otherwise
//   displays the trace message using OS debug tracing APIs.
//
//------------------------------------------------------------------------

bool XcpVTrace(
    XUINT32   uClass,          // Class of monitoring event, a.g. assertion, trace
    const WCHAR   *pFileName,
    XINT32    iLine,
    XINT32    iValue,          // E.g. assertion value, HRESULT
    const WCHAR    *pTestString,     // E.g. Assertion string
    const WCHAR    *pMessageString,  // Message and args as a string - tells us whether to look for var args
    void     *pVArgs
)
{
    bool isDebugBreak = false;
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks

#if DBG
#ifndef __LP64__
//FIX64  We can't use any of the vprint stuff until we work out the conflicting definitions of "va_list" among the various headers.
    // Support for proxying the trace messages
    IXcpTraceMessageSink* pTraceSink = NULL;
    va_list  vargs = (va_list)pVArgs;
    WCHAR    message[201];    // Fixed length message formatting buffer


    // Format varargs message string using cross-platform standard API

    message[0] = 0; // Start with an empty message buffer
    if (pMessageString && *pMessageString)
    {
        if (uClass & MonitorAnsiArgs)
        {
            uClass &= ~MonitorAnsiArgs; // Drop ANSI flag - don't send it to xcpmon.
            char   *pFormat = NULL;     // Format string from optional arguments
            XINT32  i;
            //va_start(vargs, pMessageString);
            pFormat = va_arg(vargs, char*); // The format string is the first optional argument to XcpTrace
            // Cheat: Pretend message is an ANSI buffer, then expand characters to words
            GetPALDebuggingServices()->StringCchVPrintfA((char*)message, ARRAY_SIZE(message), pFormat, vargs);
            // Expand message buffer to UTF-16 by zero filling the top of each character
            for (i=ARRAY_SIZE(message)-1; i>=0; i--)
            {
                message[i] = (WCHAR)(((XUINT8*)message)[i]);
            }
        }
        else
        {
            WCHAR *pFormat = NULL;  // Format string from optional arguments
            message[0] = 0; // Start with an empty message buffer
            //va_start(vargs, pMessageString);
            pFormat = va_arg(vargs, WCHAR*); // The format string is the first optional argument to XcpTrace
            GetPALDebuggingServices()->StringCchVPrintf(message, ARRAY_SIZE(message), pFormat, vargs);
        }
        message[ARRAY_SIZE(message)-1] = 0;  // Guarantee zero terminator
    }


    IGNOREHR(GetPALDebuggingServices()->GetTraceMessageSink(&pTraceSink));
    if (pTraceSink != NULL)
    {
        pTraceSink->AcceptTraceMessage(uClass, (WCHAR *)pFileName, iLine, iValue, (WCHAR *)pTestString, message);
    }

    if (g_pMonitorBuffer)
    {
        // SendTraceMessage can cause a delay while the monitoring process catches up.
        // Since the GetThreadMonitor recorded the clock count when obtained above, this
        // delay will not be included in performance measurements.
        SendTraceMessage(uClass, pFileName, iLine, iValue, pTestString, message, pThreadMonitor->m_clocksAtLastSuspend);
    }


    // Format the message for and send it to any debugger output window that may be attached

    DisplayDebugMessage(uClass, pFileName, iLine, iValue, pTestString, message, pThreadMonitor->m_clocksAtLastSuspend);


    // For assertion failures and corruptions, potentially issue a debugger break,
    // depending on return value of DisplayDebugDialog() - the actual debugger
    // break is the responsibility of a calling frame, as platform debugger breaks
    // tend to attribute the problem more accurately if they're done in the "correct"
    // frame (ASSERT() being one example, both locally and via WER/Watson).
    // If there is no attached debugger, displays a dialog showing the message
    // and recommending attachment of a debugger.  (Dialog part not yet implemented.)

    switch (uClass)
    {
    case MonitorAssert:
    case MonitorAssertSuccess:
    case MonitorMemoryHeaderCorruption:
    case MonitorMemoryTrailerCorruption:
        isDebugBreak = DisplayDebugDialog(uClass, pFileName, iLine, iValue, pTestString, message);

        break;
    default:
        break;
    }

#endif // not 64-bit
#endif // #if DBG

    pThreadMonitor->Resume();  // Restart perf counting

    return isDebugBreak;
}




//------------------------------------------------------------------------
//
//  Method:   XcpEnterSection
//
//  Synopsis:
//
//      Pushes the section on to the top of the section stack.
//
//------------------------------------------------------------------------

void XcpEnterSection(XUINT16 id)
{
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks

    if (    (id != METHOD_ENTRY_MARK) // Don't send method entry markers
        &&  (g_pMonitorBuffer != NULL)
        &&  (pThreadMonitor->m_sectionStackTop > 0))
    {
        // Find previous section on stcak by skipping over any method entry marks
        XUINT32 iPrevSectionStackIndex = pThreadMonitor->m_sectionStackTop-1;
        while (    (iPrevSectionStackIndex > 0)
               &&  (pThreadMonitor->m_sectionStack[iPrevSectionStackIndex] == METHOD_ENTRY_MARK))
        {
            iPrevSectionStackIndex--;
        }

        MONITORASSERT(pThreadMonitor->m_sectionStack[iPrevSectionStackIndex] != METHOD_ENTRY_MARK);

        // Send message with this threads clock time and section being left
        SendTraceMessage(
            MonitorEnterSection,
            NULL,          // No file name
            id,            // Section being entered (not line number)
            pThreadMonitor->m_sectionStack[iPrevSectionStackIndex],
            NULL,          // No test string
            NULL,          // No message
            // Application clocks in this thread:
            pThreadMonitor->m_clocksAtLastSuspend - pThreadMonitor->m_clocksWhileSuspended
        );
    }

    pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop] = id;

    MONITORASSERT(pThreadMonitor->m_sectionStackTop < ARRAY_SIZE(pThreadMonitor->m_sectionStack));

    pThreadMonitor->m_sectionStackTop++;

    pThreadMonitor->Resume();  // Restart perf counting
}




//------------------------------------------------------------------------
//
//  Method:   XcpLeaveSection
//
//  Synopsis:
//
//      Pops the section from the top of the section stack.
// 
//------------------------------------------------------------------------

void XcpLeaveSection(XUINT16 id)
{
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks

    MONITORASSERT(pThreadMonitor->m_sectionStackTop > 0);

    if (g_pMonitorBuffer)
    {
        // Send message with this threads clock time and section being left
        SendTraceMessage(
            MonitorLeaveSection,
            NULL,          // No file name
            0,             // No line number
            pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop-1], // Section being left
            NULL,          // No test string
            NULL,          // No message
            // Application clocks in this thread:
            pThreadMonitor->m_clocksAtLastSuspend - pThreadMonitor->m_clocksWhileSuspended
        );
    }

    pThreadMonitor->m_sectionStackTop--;

    MONITORASSERT(pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop] == id);

    pThreadMonitor->Resume();  // Restart perf counting
}



//------------------------------------------------------------------------
//
//  Method:   XcpPopToMark
//
//  Synopsis:
//
//      Pops entries until it reaches METHOD_ENTRY_MARK.
//
//------------------------------------------------------------------------

void XcpPopToMark()
{
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor(); // Suspend counting clocks

    // Send MonitorLeaveSection message for all stacked sections
    // until we find a METHOD_ENTRY_MARK

    while (    (pThreadMonitor->m_sectionStackTop > 0)
           &&  (pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop-1] != METHOD_ENTRY_MARK))
    {
        if (g_pMonitorBuffer)
        {
            // Send message with this threads clock time and section being left
            SendTraceMessage(
                MonitorLeaveSection,
                NULL,          // No file name
                0,             // No line number
                pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop-1], // Section being left
                NULL,          // No test string
                NULL,          // No message
                // Application clocks in this thread:
                pThreadMonitor->m_clocksAtLastSuspend - pThreadMonitor->m_clocksWhileSuspended
            );
        }
        pThreadMonitor->m_sectionStackTop--;
    }

    // We should now be at the method entry mark

    MONITORASSERT(pThreadMonitor->m_sectionStackTop > 0);
    MONITORASSERT(pThreadMonitor->m_sectionStack[pThreadMonitor->m_sectionStackTop-1] == METHOD_ENTRY_MARK);
    pThreadMonitor->m_sectionStackTop--;  // Pop the method entry mark


    pThreadMonitor->Resume();  // Restart perf counting
}




//-------------------------------------------------------------------------
//
//  Function:   FlatXcpTrace
//
//  Synopsis:
//     Flat version of XcpTrace for use by C code
//
//-------------------------------------------------------------------------


extern "C" bool FlatXcpTrace(
    XUINT32  flag,
    XUINT32  uClass,          // Class of monitoring event, a.g. assertion, trace
    WCHAR   *pFilename,
    XINT32   iLine,
    XINT32   iValue,          // E.g. assertion value, HRESULT
    WCHAR    *pTestString,    // E.g. Assertion string
    WCHAR    *pMessageString, // Message and args as a string - tells us whether to look for var args
    ...
)
{
    va_list pVArgs;
    if (GetPALDebuggingServices()->GetTraceFlags() & flag)
    {
        va_start(pVArgs, pMessageString);
        return GetPALDebuggingServices()->XcpVTrace(uClass,pFilename,iLine,iValue,pTestString,pMessageString,pVArgs);
    }
    return false;
}



#endif // #if XCP_MONITOR
