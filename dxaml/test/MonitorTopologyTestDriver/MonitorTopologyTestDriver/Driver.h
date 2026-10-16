// SPDX-License-Identifier: MS-PL
// Copyright (c) Microsoft Corporation.
// Derived from Windows-driver-samples, commit
// 3c3fb49073c047c4cc8e6c203c6331f62b426507, video/IndirectDisplay/IddSampleDriver.
// Changes: bounded, session-owned, EDID-less virtual monitors for Topology Test.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <wudfwdm.h>
#include <wdf.h>
#include <iddcx.h>
#include <dxgi1_5.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <new>
#include <stdint.h>
#include "..\shared\TopologyDriverProtocol.h"
#include "Trace.h"

class SwapChainWorker final
{
public:
    SwapChainWorker(IDDCX_SWAPCHAIN chain, LUID renderAdapter, HANDLE frameEvent) noexcept;
    ~SwapChainWorker() noexcept;
    HRESULT Prepare() noexcept;
    void Accept() noexcept;
    void Stop() noexcept;

    SwapChainWorker(const SwapChainWorker&) = delete;
    SwapChainWorker& operator=(const SwapChainWorker&) = delete;

private:
    static DWORD WINAPI ThreadEntry(void* argument) noexcept;
    HRESULT Run() noexcept;

    IDDCX_SWAPCHAIN m_chain;
    LUID m_renderAdapter;
    HANDLE m_frameEvent;
    HANDLE m_startEvent = nullptr;
    HANDLE m_stopEvent = nullptr;
    HANDLE m_thread = nullptr;
    bool m_ownsChain = false;
};

// WDF zero-initializes these POD contexts. No constructors/destructors are assumed.
struct MonitorState
{
    volatile LONG References;
    SRWLOCK Lock;
    CONDITION_VARIABLE Idle;
    SwapChainWorker* Worker;
    uint64_t Epoch;
    UINT Assignments;
    UINT RetiringWorkers;
    bool Closing;
    volatile LONG Gone;
    MTT_MONITOR Wire;
    DISPLAYCONFIG_VIDEO_SIGNAL_INFO Signal;
};

struct MonitorContext
{
    SRWLOCK Lock;
    MonitorState* State;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MonitorContext, GetMonitorContext);

struct DeviceContext;
struct AdapterContext
{
    SRWLOCK Lock;
    DeviceContext* Device;
    bool Finished;
    NTSTATUS InitStatus;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(AdapterContext, GetAdapterContext);

struct MonitorSlot
{
    IDDCX_MONITOR Object;
    MonitorState* State;
    MTT_MONITOR Wire;
};

struct DeviceContext
{
    // Only control, file, and power callbacks take this gate. IddCx callbacks
    // and child cleanup callbacks NEVER take it, including during departure.
    SRWLOCK Gate;
    WDFDEVICE Device;
    IDDCX_ADAPTER Adapter;
    volatile LONG AdapterFinished;
    volatile LONG AdapterGone;
    NTSTATUS AdapterStatus;
    WDFFILEOBJECT Owner;
    MonitorSlot Slots[MTT_MAX_MONITORS];
    uint32_t Generation;
    bool Powered;
    bool Orphaned;
    bool Removing;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DeviceContext, GetDeviceContext);

struct FileContext
{
    bool IsController;
};
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FileContext, GetFileContext);

class ExclusiveLock final
{
public:
    explicit ExclusiveLock(SRWLOCK& lock) noexcept : m_lock(lock)
    {
        AcquireSRWLockExclusive(&m_lock);
    }
    ~ExclusiveLock() noexcept { ReleaseSRWLockExclusive(&m_lock); }
    ExclusiveLock(const ExclusiveLock&) = delete;
    ExclusiveLock& operator=(const ExclusiveLock&) = delete;
private:
    SRWLOCK& m_lock;
};
