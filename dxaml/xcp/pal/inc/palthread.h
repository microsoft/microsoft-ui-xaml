// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//--------------------------------------------------------
//
// PAL Threading Data Interfaces
//
//--------------------------------------------------------

//------------------------------------------------------------------------
//
//  Interface:  IPALEvent
//
//  Synopsis:
//      PAL interface for a signable event.
//
//------------------------------------------------------------------------

struct IPALEvent : public IPALWaitable
{
    virtual _Check_return_ HRESULT Set() = 0;
    virtual _Check_return_ HRESULT Reset() = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALQueue
//
//  Synopsis:
//      PAL interface for a queue.
//
//------------------------------------------------------------------------

struct IPALQueue : public IPALWaitable
{
    virtual void Post(_In_ void *pAddress) = 0;
    virtual _Check_return_ HRESULT Get(_Outptr_ void **ppAddress, _In_ XUINT32 nWait) = 0;
    virtual _Check_return_ HRESULT Remove(_In_ void *pAddress) = 0;
};

typedef XINT32 (__stdcall *PALTHREADPFN)(_In_ XUINT8 *pData);



//------------------------------------------------------------------------
//
//  Enum:  PAL_IPCInitMode
//
//  Synopsis:
//      Enum describing the ways of initializing cross-process PAL IPC 
//      objects (shared memory, named mutex, etc.).
//
//      InitModeCreateOnly means the IPC resource should only be 
//      created. If it already exists, the initialization function must
//      fail.
//
//      InitModeOpenOnly means the IPC resource must not be created.
//      If it doesn't already exist, the initialization function must
//      fail.
//
//      InitModeOpenOrCreate means the resource will be created if it does
//      not already exist. If it does exist, a handle to the existing
//      resource will be returned. When using OpenOrCreate, it isn't 
//      possible to determine whether the resource was created or just 
//      opened (not all platforms support this).
//
//------------------------------------------------------------------------
enum PAL_IPCInitMode
{
    InitModeCreateOnly,
    InitModeOpenOnly,
    InitModeOpenOrCreate
};


//------------------------------------------------------------------------
//
//  Interface:  IPALThreadingServices
//
//  Synopsis:
//      Provides an abstraction for system threading support.
//
//------------------------------------------------------------------------
struct __declspec(novtable) IPALThreadingServices
{
// Threading and synchronization services

    virtual HRESULT ThreadCreate(
        _Outptr_ IPALWaitable **ppThread,
        _In_ PALTHREADPFN pfn,
        _In_ XUINT32 cData,
        _In_reads_(cData) XUINT8 *pData) = 0;

    virtual HRESULT EventCreate(
        _Outptr_ IPALEvent **ppEvent,
        _In_ XINT32 bSignaled,
        _In_ XINT32 bManual) = 0;

    virtual HRESULT QueueCreate(
        _Outptr_ IPALQueue **ppQueue) = 0;

    virtual _Check_return_ HRESULT NamedEventCreate(
        _Outptr_ IPALEvent** ppEvent,
        PAL_IPCInitMode initMode,
        bool bInitialState,
        bool bManualReset,
        _In_ const xstring_ptr_view& strName,
        bool bReturnFailureIfCreationFailed) = 0;

    virtual XUINT32 WaitForObjects(
        _In_ XUINT32 cWaitable,
        _In_ IPALWaitable **ppWaitable,
        _In_ XINT32 bAll,
        _In_ XUINT32 nTimeout) = 0;

    virtual XINT32  ThreadGetPriority(_In_opt_ IPALWaitable *pThread) = 0;
    virtual HRESULT ThreadSetPriority(_In_opt_ IPALWaitable *pThread, _In_ XINT32 nPriority) = 0;
    virtual XINT32  ProcessGetPriority() = 0;
    virtual HRESULT ProcessSetPriority(_In_ XINT32 nPriority) = 0;
};

XINT32 PAL_InterlockedCompareExchange(
    _Inout_ XINT32 *pTarget,
    _In_ XINT32 Exchange,
    _In_ XINT32 Comperand);

XINT32 PAL_InterlockedDecrement(_Inout_ XINT32 *pnTarget);

XINT32 PAL_InterlockedExchange(
    _Inout_ XINT32 *pnTarget,
    _In_ XINT32 nValue);

XINT32 PAL_InterlockedIncrement(_Inout_ XINT32 *pnTarget);


//------------------------------------------------------------------------
//
//  Interface:  xinit_cross_thread
//
//  Synopsis:
//      Container class for a value that is set on one thread
//      and read on another.  The MemoryBarrier macro is used to ensure
//      that neither the compiler nor the CPU reorders the memory writes.
//      If a processor sees m_initialized == TRUE
//      then m_value has been set.
//
//------------------------------------------------------------------------
template<typename T>
class xinit_cross_thread
{
public:
    xinit_cross_thread() 
        : m_initialized(FALSE)
    {
    }

    void set(
        _In_ T *pVal
        )
    {
        // This should only be called once
        ASSERT(!m_initialized);

        m_value = *pVal;

        // Ensure that the write to m_initialized is only visible to a processor
        // after the write to m_value is visible
        MemoryBarrier();

        m_initialized = TRUE;
    }

    bool get(
        _Out_opt_ T* pVal
        )
    {
        bool result = false;

        if (m_initialized)
        {
            *pVal = m_value;
            result = TRUE;
        }

        return result;
    }

private:
    T m_value;
    bool m_initialized;
};
