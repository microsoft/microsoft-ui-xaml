// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Provides trace and monitoring services in debug builds only.
//      This file is shared between monitor.cpp (part of agwin.dll) and agmon.exe.

#ifndef MONITOR_BUFFER_H
#define MONITOR_BUFFER_H

#include "paldebugging.h"

#ifndef MONITORASSERT
    #if DBG
// The whole purpose of this macro is to get expansion of a string to a long string to work
// on the Mac. It seems that L#"" expands directly to L "" giving us a free L that doesn't compile.
// By calling this macro with an argument of #x we get a correct L"xxx" string declaration.
        #define MACW(x) L##x
        #define MONITORASSERT(x)                                        \
            do {                                                        \
                if (!(x))                                               \
                {                                                       \
                    GetPALDebuggingServices()->DebugOutputSz(MACW(#x)); \
                    FAIL_ASSERT_HERE(MACW(#x));                         \
                }                                                       \
            } while(0)
    #else
        #define MONITORASSERT(X)
    #endif
#endif




//------------------------------------------------------------------------
//
//  Struct:   MonitorMessageHeader
//
//  Synopsis:
//
//      All messages sent to xcpmon.exe include this header.
//
//------------------------------------------------------------------------

struct MonitorMessageHeader
{
    XUINT32  type;           // 0 during construction, 1 when ready.
    XUINT32  length;         // Total length of message, including this header
    XUINT64  processClocks;  // Measured cycles
    XDOUBLE  runTime;        // Time since start in seconds
    DWORD    threadID;
};








//------------------------------------------------------------------------
//
//  Struct:   MonitorBuffer
//
//  Synopsis:
//
//      Describes the layout of the shared memory section that provides the
//      communication channel between the running XCP app and xcpmon.exe.
//
//------------------------------------------------------------------------

__declspec(align(8)) struct MonitorBuffer
{
public:
    XUINT32         m_traceFlags;
    XUINT32         m_processId;
    volatile XINT32 m_bInUse;         // Set non-zero by the first taker.
    XUINT64         m_fileHandle;     // Platform independent slot to retain data for closing mapped memory

protected:
    XINT32          m_inLocked;       // Semaphore controlling writing to the buffer
    volatile XUINT32 m_in;             // Circular buffer input offset
    XUINT32          m_out;            // Circular buffer output offset
    XUINT8          m_buffer[65536];  // The circular buffer

public:

    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer constructor
    //
    //  Synopsis:
    //
    //      Creates an empty monitor buffer structure.
    //------------------------------------------------------------------------

    void Initialize()
    {
        m_processId  = 0;
        m_bInUse     = 0;   // Set non-zero by the first taker.
        m_inLocked   = 0;
        m_in         = 0;
        m_out        = 0;
        m_traceFlags = TraceAlways;
    }




    //------------------------------------------------------------------------
    //
    //  Circular buffer defintions:
    //
    //      Invariant:
    //          In  >= 0
    //          In  <  buffersize,
    //          Out >= 0,
    //          out <  buffersize
    //
    //      Empty: In = Out
    //
    //      Full:  (In = Out-1) OR (In = buffersize-1 AND Out = 0)
    //
    //      A full buffer contains buffersize-1 XUINT8s.
    //
    //
    //  Circular buffer access protocol:
    //
    //      Single reader process (xcpmon.exe)
    //
    //          The reader always checks first available XUINT32 at (Out) and waits for
    //          it to go non-zero before reading and advancing over the record.
    //
    //      Multiple writers:
    //
    //          Writing support code not defined here. See WriteableMonitorBuffer
    //          in monitor.cpp.
    //
    //          Each thread in the process is one writer
    //
    //          Writers synchronized via LockWrites/ReleaseWrites calls (see above)
    //
    //          Writing done by reserving space after In.
    //
    //          All records start with a message type, valid types are always non-zero.
    //
    //          Reserve initially sets the type to zero before releasing the write lock.
    //
    //          Each writer fills in its reserved space, and only sets the type non-zero
    //          when eveything else in the record is complete.
    //
    //          Pointers are updated with Safe* APIs to guarantee no
    //          partial updates.
    //
    //          Since multiple writers could be writing at once, there can be multiple
    //          incomplete records at once.
    //
    //------------------------------------------------------------------------


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::TotalAvailableOut
    //
    //  Synopsis:
    //
    //      Returns number of bytes of buffer space that contain data ready
    //      for reading.
    //
    //------------------------------------------------------------------------

    XUINT32 TotalAvailableOut()
    {
        XUINT32 iIn = m_in;

        if (iIn >= m_out)
            return iIn - m_out;
        else
            return iIn + sizeof(m_buffer) - m_out;
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::ContiguousAvailableOut
    //
    //  Synopsis:
    //
    //      Returns number of contiguous bytes of buffer space that contain
    //      data ready for reading.
    //
    //------------------------------------------------------------------------

    XUINT32 ContiguousAvailableOut()
    {
        XUINT32 iIn = m_in;

        if (iIn >= m_out)
            return iIn - m_out;
        else
            return sizeof(m_buffer) - m_out;
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::pOut
    //
    //  Synopsis:
    //
    //      Returns address of first byte available for reading.
    //
    //------------------------------------------------------------------------

    void *pOut()
    {
        return (void*)&m_buffer[m_out];
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::AdvanceOut
    //
    //  Synopsis:
    //
    //      Advances the out pointer, wrapping around the end of the buffer
    //      as necessary.
    //
    //------------------------------------------------------------------------

    void AdvanceOut(XUINT32 cAdvance)
    {
        MONITORASSERT(cAdvance <= TotalAvailableOut());

        XUINT32 newOut = m_out += cAdvance;

        if (newOut < sizeof(m_buffer))
        {
            m_out = newOut;
        }
        else
        {
            m_out = newOut - sizeof(m_buffer);
        }
    }




    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::TotalAvailableIn
    //
    //  Synopsis:
    //
    //      Returns number of bytes of buffer space that are available for data
    //      to be added.
    //
    //  Notes:
    //
    //      A full buffer always has at least one unsed byte in order to
    //      distinguish it from an empty buffer.
    //
    //------------------------------------------------------------------------

    XUINT32 TotalAvailableIn()
    {
        XUINT32 iIn = m_in;

        if (m_out > iIn)
            return m_out - iIn - 1;
        else
            return m_out + sizeof(m_buffer) - iIn - 1;
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::ContiguousAvailableIn
    //
    //  Synopsis:
    //
    //      Returns number of contiguous bytes of buffer space that are available
    //      for data to be added.
    //
    //      Note the special case when the contiguous space is at the end of the
    //      buffer but more space is available at the beginning of the buffer.
    //
    //------------------------------------------------------------------------

    XUINT32 ContiguousAvailableIn()
    {
        XUINT32 iIn = m_in;

        if (m_out > iIn)
            return m_out - iIn - 1;
        else
            if (m_out > 0)
                return sizeof(m_buffer) - iIn;
            else
                return sizeof(m_buffer) - iIn - 1;
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::pIn
    //
    //  Synopsis:
    //
    //      Returns the address of the first byte at which data can be added
    //      to the buffer.
    //
    //------------------------------------------------------------------------

    void *pIn()
    {
        return (void*)&m_buffer[m_in];
    }




    // Data transfer functions


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::PeekBytes
    //
    //  Synopsis:
    //
    //      Copies the requested nuber of bytes from the circular buffer
    //      without advancing the buffer pointer.
    //
    //------------------------------------------------------------------------

    void PeekBytes(XUINT32 cSize, void *pTarget)
    {
        MONITORASSERT(cSize <= TotalAvailableOut());
        XUINT8 *pbTarget = (XUINT8*) pTarget;

        // Copy as much as possible, up to any buffer wrap around

        XUINT32 available = ContiguousAvailableOut();
        XUINT32 firstCopySize = cSize < available ? cSize : available;
        memcpy(pbTarget, pOut(), firstCopySize);

        // Copy any remaining available bytes from the start of the buffer

        if (cSize > firstCopySize)
        {
            // Copy the remainder
            cSize -= firstCopySize;
            pbTarget += firstCopySize;
            memcpy(pbTarget, m_buffer, cSize);
        }
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::ReadBytes
    //
    //  Synopsis:
    //
    //      Copies the requested number of bytes from the circular buffer
    //      and advances the out pointer correspondingly.
    //
    //------------------------------------------------------------------------

    void ReadBytes(XUINT32 cSize, void *pTarget)
    {
        PeekBytes(cSize, pTarget);

        if (ContiguousAvailableOut() < cSize)
        {
            cSize -= ContiguousAvailableOut();
            AdvanceOut(ContiguousAvailableOut());
        }

        AdvanceOut(cSize);
    }


    //------------------------------------------------------------------------
    //
    //  Method:   MonitorBuffer::PeekType
    //
    //  Synopsis:
    //
    //      Returns the value of the XUINT32 available at Out, or 0 if none is
    //      available.
    //
    //------------------------------------------------------------------------

    XUINT32 PeekType()
    {
        // Return first 4 XUINT8s as XUINT32 if available, 0 otherwise

        if (TotalAvailableOut() < sizeof(XUINT32))
        {
            return 0;
        }

        XUINT32 type = 0;
        PeekBytes(sizeof(type), &type);

        return type;
    }
};

#endif //#ifndef MONITOR_BUFFER_H
