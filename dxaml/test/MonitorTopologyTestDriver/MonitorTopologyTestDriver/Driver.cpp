// SPDX-License-Identifier: MS-PL
// Copyright (c) Microsoft Corporation.
// Derived from Windows-driver-samples, commit
// 3c3fb49073c047c4cc8e6c203c6331f62b426507, video/IndirectDisplay/IddSampleDriver.
// This MS-PL implementation consumes the separate MIT-licensed shared ABI.
#include "Driver.h"

using Microsoft::WRL::ComPtr;

extern "C" DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD TopologyDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY TopologyD0Entry;
EVT_WDF_DEVICE_D0_EXIT TopologyD0Exit;
EVT_WDF_DEVICE_FILE_CREATE TopologyFileCreate;
EVT_WDF_FILE_CLEANUP TopologyFileCleanup;
EVT_WDF_OBJECT_CONTEXT_CLEANUP TopologyDeviceCleanup;
EVT_WDF_OBJECT_CONTEXT_CLEANUP TopologyMonitorCleanup;
EVT_WDF_OBJECT_CONTEXT_CLEANUP TopologyAdapterCleanup;
EVT_WDF_OBJECT_CONTEXT_CLEANUP TopologyDriverCleanup;
EVT_IDD_CX_DEVICE_IO_CONTROL TopologyIoControl;
EVT_IDD_CX_ADAPTER_INIT_FINISHED TopologyAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES TopologyCommitModes;
EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION TopologyParseDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES TopologyDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES TopologyTargetModes;
EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN TopologyAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN TopologyUnassignSwapChain;

namespace
{
constexpr wchar_t ControlReference[] = L"TopologyControl";

bool IsControlFileName(PCUNICODE_STRING name) noexcept
{
    if (!name || !name->Buffer || name->Length % sizeof(wchar_t) != 0)
    {
        return false;
    }
    const wchar_t* text = name->Buffer;
    int length = name->Length / sizeof(wchar_t);
    if (length != 0 && text[0] == L'\\')
    {
        ++text;
        --length;
    }
    // Device-interface reference strings can be normalized by the I/O stack.
    return CompareStringOrdinal(text, length, ControlReference,
        ARRAYSIZE(ControlReference) - 1, TRUE) == CSTR_EQUAL;
}

// Stable slot identities, not fabricated EDIDs or physical monitor properties.
// Use one test adapter per machine.
constexpr GUID MonitorContainers[MTT_MAX_MONITORS] =
{
    { 0x8fdbe590, 0x7d13, 0x4898, { 0x8c, 0x9a, 0x39, 0x16, 0x8a, 0xc2, 0x2f, 0x10 } },
    { 0x8fdbe590, 0x7d13, 0x4898, { 0x8c, 0x9a, 0x39, 0x16, 0x8a, 0xc2, 0x2f, 0x11 } },
    { 0x8fdbe590, 0x7d13, 0x4898, { 0x8c, 0x9a, 0x39, 0x16, 0x8a, 0xc2, 0x2f, 0x12 } },
    { 0x8fdbe590, 0x7d13, 0x4898, { 0x8c, 0x9a, 0x39, 0x16, 0x8a, 0xc2, 0x2f, 0x13 } }
};

LONG ReadAtomic(volatile LONG* value) noexcept
{
    return InterlockedCompareExchange(value, 0, 0);
}

uint64_t Gcd(uint64_t a, uint64_t b) noexcept
{
    while (b != 0)
    {
        const uint64_t next = a % b;
        a = b;
        b = next;
    }
    return a;
}

NTSTATUS BuildSignal(const MTT_ADD_REQUEST& request,
    DISPLAYCONFIG_VIDEO_SIGNAL_INFO& signal) noexcept
{
    if (request.Id >= MTT_MAX_MONITORS ||
        request.Width < MTT_MIN_DIMENSION || request.Width > MTT_MAX_DIMENSION ||
        request.Height < MTT_MIN_DIMENSION || request.Height > MTT_MAX_DIMENSION ||
        request.RefreshNumerator == 0 || request.RefreshDenominator == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    const uint64_t numerator = request.RefreshNumerator;
    const uint64_t denominator = request.RefreshDenominator;
    if (numerator < MTT_MIN_REFRESH_HZ * denominator ||
        numerator > MTT_MAX_REFRESH_HZ * denominator)
    {
        return STATUS_INVALID_PARAMETER;
    }

    const uint64_t horizontalNumerator = numerator * request.Height;
    const uint64_t horizontalDivisor = Gcd(horizontalNumerator, denominator);
    if (horizontalNumerator / horizontalDivisor > UINT32_MAX)
    {
        // DISPLAYCONFIG_RATIONAL has 32-bit fields. Do not silently truncate
        // pathological fractions that cannot represent the horizontal rate.
        return STATUS_NOT_SUPPORTED;
    }

    signal = {};
    signal.activeSize.cx = signal.totalSize.cx = request.Width;
    signal.activeSize.cy = signal.totalSize.cy = request.Height;
    signal.vSyncFreq.Numerator = request.RefreshNumerator;
    signal.vSyncFreq.Denominator = request.RefreshDenominator;
    signal.hSyncFreq.Numerator = static_cast<UINT32>(horizontalNumerator / horizontalDivisor);
    signal.hSyncFreq.Denominator = static_cast<UINT32>(denominator / horizontalDivisor);
    // Like the upstream virtual transport, there is no physical blanking.
    // pixelRate is integral Hz; round to nearest while keeping exact rational
    // vertical/horizontal frequencies (e.g. 60000/1001, not integer 59 or 60).
    signal.pixelRate = (horizontalNumerator * request.Width + denominator / 2) / denominator;
    signal.AdditionalSignalInfo.videoStandard = 255;
    signal.AdditionalSignalInfo.vSyncFreqDivider = 0; // Monitor-description mode.
    signal.scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
    return STATUS_SUCCESS;
}

void ReleaseMonitorState(MonitorState* monitor) noexcept
{
    if (InterlockedDecrement(&monitor->References) == 0)
    {
        delete monitor;
    }
}

class MonitorReference final
{
public:
    explicit MonitorReference(IDDCX_MONITOR object) noexcept
    {
        auto* context = GetMonitorContext(object);
        ExclusiveLock lock(context->Lock);
        m_state = context->State;
        if (m_state)
        {
            InterlockedIncrement(&m_state->References);
        }
    }
    ~MonitorReference() noexcept
    {
        if (m_state) ReleaseMonitorState(m_state);
    }
    MonitorState* Get() const noexcept { return m_state; }
    MonitorReference(const MonitorReference&) = delete;
    MonitorReference& operator=(const MonitorReference&) = delete;
private:
    MonitorState* m_state = nullptr;
};

void StopMonitor(MonitorState* monitor, bool permanent) noexcept
{
    AcquireSRWLockExclusive(&monitor->Lock);
    monitor->Closing = monitor->Closing || permanent;
    ++monitor->Epoch;
    SwapChainWorker* worker = monitor->Worker;
    monitor->Worker = nullptr;
    if (worker)
    {
        ++monitor->RetiringWorkers;
    }
    ReleaseSRWLockExclusive(&monitor->Lock);

    // Neither the monitor lock nor a WDF synchronization lock is held across
    // join/delete. The worker can call IddCx and delete its owned swapchain.
    delete worker;

    AcquireSRWLockExclusive(&monitor->Lock);
    if (worker)
    {
        --monitor->RetiringWorkers;
        WakeAllConditionVariable(&monitor->Idle);
    }
    if (permanent)
    {
        while (monitor->Assignments != 0 || monitor->RetiringWorkers != 0)
        {
            SleepConditionVariableSRW(&monitor->Idle, &monitor->Lock, INFINITE, 0);
        }
    }
    ReleaseSRWLockExclusive(&monitor->Lock);
}

uint32_t MonitorCount(const DeviceContext* device) noexcept
{
    uint32_t count = 0;
    for (const auto& slot : device->Slots)
    {
        count += slot.Object != nullptr ? 1u : 0u;
    }
    return count;
}

// The slot and WDF context own separate references to ordinary C++ state.
// Never access WDF context storage after its EvtCleanupCallback has run.
void ReapDestroyedMonitors(DeviceContext* device) noexcept
{
    for (auto& slot : device->Slots)
    {
        if (slot.Object && ReadAtomic(&slot.State->Gone))
        {
            auto* state = slot.State;
            slot = {};
            ++device->Generation;
            ReleaseMonitorState(state);
        }
    }
    if (!device->Owner && MonitorCount(device) == 0)
    {
        device->Orphaned = false;
    }
}

bool IsReady(DeviceContext* device) noexcept
{
    if (!device->Powered || device->Removing || device->Orphaned || !device->Adapter)
    {
        return false;
    }
    return !ReadAtomic(&device->AdapterGone) && ReadAtomic(&device->AdapterFinished) &&
        NT_SUCCESS(device->AdapterStatus);
}

NTSTATUS RemoveMonitor(DeviceContext* device, uint32_t id) noexcept
{
    auto& slot = device->Slots[id];
    if (!slot.Object)
    {
        return STATUS_NOT_FOUND;
    }
    const auto object = slot.Object;
    auto* monitor = slot.State;
    StopMonitor(monitor, true);
    NTSTATUS status = STATUS_SUCCESS;
    if (!ReadAtomic(&monitor->Gone))
    {
        // Successful departure destroys the IddCx monitor. Never follow it
        // with WdfObjectDelete: https://learn.microsoft.com/windows-hardware/drivers/display/iddcx-objects
        status = IddCxMonitorDeparture(object);
    }
    if (!NT_SUCCESS(status))
    {
        LogFailure(L"IddCxMonitorDeparture (slot retained)", status);
        return status;
    }
    slot = {};
    ++device->Generation;
    ReleaseMonitorState(monitor);
    return STATUS_SUCCESS;
}

NTSTATUS ClearMonitors(DeviceContext* device) noexcept
{
    ReapDestroyedMonitors(device);
    NTSTATUS firstFailure = STATUS_SUCCESS;
    for (uint32_t id = 0; id != MTT_MAX_MONITORS; ++id)
    {
        if (device->Slots[id].Object)
        {
            const NTSTATUS status = RemoveMonitor(device, id);
            if (!NT_SUCCESS(status) && NT_SUCCESS(firstFailure))
            {
                firstFailure = status;
            }
        }
    }
    return firstFailure;
}

NTSTATUS AddMonitor(DeviceContext* device, const MTT_ADD_REQUEST& request,
    const DISPLAYCONFIG_VIDEO_SIGNAL_INFO& signal, MTT_MONITOR& result) noexcept
{
    if (device->Slots[request.Id].Object)
    {
        return STATUS_OBJECT_NAME_COLLISION;
    }
    auto* monitor = new (std::nothrow) MonitorState{};
    if (!monitor)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    monitor->References = 1; // Transferred into the device slot after arrival.
    monitor->Signal = signal;
    monitor->Wire.Id = request.Id;
    monitor->Wire.Width = request.Width;
    monitor->Wire.Height = request.Height;
    monitor->Wire.RefreshNumerator = request.RefreshNumerator;
    monitor->Wire.RefreshDenominator = request.RefreshDenominator;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, MonitorContext);
    attributes.EvtCleanupCallback = TopologyMonitorCleanup;
    attributes.SynchronizationScope = WdfSynchronizationScopeNone;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    IDDCX_MONITOR_INFO info = {};
    info.Size = sizeof(info);
    info.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED;
    info.ConnectorIndex = request.Id;
    info.MonitorContainerId = MonitorContainers[request.Id];
    // IddCx supports descriptor-less monitors through DefaultDescriptionModes.
    // No fabricated EDID, physical size, DPI, or monitor vendor identity.
    info.MonitorDescription.Size = sizeof(info.MonitorDescription);
    info.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
    info.MonitorDescription.DataSize = 0;
    info.MonitorDescription.pData = nullptr;

    IDARG_IN_MONITORCREATE create = {};
    create.ObjectAttributes = &attributes;
    create.pMonitorInfo = &info;
    IDARG_OUT_MONITORCREATE created = {};
    NTSTATUS status = IddCxMonitorCreate(device->Adapter, &create, &created);
    if (!NT_SUCCESS(status))
    {
        LogFailure(L"IddCxMonitorCreate", status);
        ReleaseMonitorState(monitor);
        return status;
    }

    const auto object = created.MonitorObject;
    InterlockedIncrement(&monitor->References); // Released by monitor cleanup.
    {
        auto* context = GetMonitorContext(object);
        ExclusiveLock lock(context->Lock);
        context->State = monitor;
    }

    IDARG_OUT_MONITORARRIVAL arrival = {};
    status = IddCxMonitorArrival(object, &arrival);
    if (!NT_SUCCESS(status))
    {
        LogFailure(L"IddCxMonitorArrival", status);
        // Arrival did not transfer the monitor into the live topology.
        // Delete the unpublished object, not a successfully departed monitor.
        WdfObjectDelete(object);
        ReleaseMonitorState(monitor);
        return status;
    }

    monitor->Wire.AdapterLuidLow = arrival.OsAdapterLuid.LowPart;
    monitor->Wire.AdapterLuidHigh = arrival.OsAdapterLuid.HighPart;
    monitor->Wire.TargetId = arrival.OsTargetId;
    device->Slots[request.Id].Object = object;
    device->Slots[request.Id].State = monitor;
    device->Slots[request.Id].Wire = monitor->Wire;
    ++device->Generation;
    result = monitor->Wire;
    return STATUS_SUCCESS;
}

template<typename T>
NTSTATUS ReadRequest(WDFREQUEST request, size_t inputLength, T& value) noexcept
{
    if (inputLength != sizeof(T))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    void* buffer = nullptr;
    NTSTATUS status = WdfRequestRetrieveInputBuffer(request, sizeof(T), &buffer, nullptr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    // METHOD_BUFFERED input/output alias. Copy before writing any output.
    memcpy(&value, buffer, sizeof(value));
    if (value.Size != sizeof(T))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    if (value.Version != MTT_PROTOCOL_VERSION)
    {
        return STATUS_REVISION_MISMATCH;
    }
    return STATUS_SUCCESS;
}

NTSTATUS ControlRequest(DeviceContext* device, WDFREQUEST request,
    size_t outputLength, size_t inputLength, ULONG code, ULONG_PTR& information) noexcept
{
    information = 0;
    MTT_REQUEST header = {};
    MTT_ADD_REQUEST add = {};
    MTT_REMOVE_REQUEST remove = {};
    DISPLAYCONFIG_VIDEO_SIGNAL_INFO signal = {};
    size_t requiredOutput = 0;
    NTSTATUS status;
    switch (code)
    {
    case IOCTL_MTT_GET_STATE:
        status = ReadRequest(request, inputLength, header);
        requiredOutput = sizeof(MTT_STATE);
        break;
    case IOCTL_MTT_ADD_MONITOR:
        status = ReadRequest(request, inputLength, add);
        requiredOutput = sizeof(MTT_MONITOR);
        if (NT_SUCCESS(status))
        {
            status = BuildSignal(add, signal);
        }
        break;
    case IOCTL_MTT_REMOVE_MONITOR:
        status = ReadRequest(request, inputLength, remove);
        if (NT_SUCCESS(status) && remove.Id >= MTT_MAX_MONITORS)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    case IOCTL_MTT_CLEAR_MONITORS:
        status = ReadRequest(request, inputLength, header);
        break;
    default:
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    // V1 has no input reserved fields. Exact lengths reject trailing fields.
    if (outputLength != requiredOutput)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    void* output = nullptr;
    if (requiredOutput != 0)
    {
        status = WdfRequestRetrieveOutputBuffer(request, requiredOutput, &output, nullptr);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    ExclusiveLock lock(device->Gate);
    const auto file = WdfRequestGetFileObject(request);
    if (!file || device->Owner != file || !GetFileContext(file)->IsController)
    {
        return STATUS_ACCESS_DENIED;
    }
    if (device->Removing)
    {
        return STATUS_DELETE_PENDING;
    }
    ReapDestroyedMonitors(device);
    if (code == IOCTL_MTT_GET_STATE)
    {
        MTT_STATE state = {};
        state.Size = sizeof(state);
        state.Version = MTT_PROTOCOL_VERSION;
        state.Ready = IsReady(device) ? 1u : 0u;
        state.MaxMonitors = MTT_MAX_MONITORS;
        state.Generation = device->Generation;
        for (const auto& slot : device->Slots)
        {
            if (slot.Object)
            {
                state.Monitors[state.MonitorCount++] = slot.Wire;
            }
        }
        memcpy(output, &state, sizeof(state));
        information = sizeof(state);
        return STATUS_SUCCESS;
    }
    if (!IsReady(device))
    {
        return STATUS_DEVICE_NOT_READY;
    }
    if (WdfRequestIsCanceled(request))
    {
        return STATUS_CANCELLED;
    }
    switch (code)
    {
    case IOCTL_MTT_ADD_MONITOR:
    {
        MTT_MONITOR result = {};
        status = AddMonitor(device, add, signal, result);
        if (NT_SUCCESS(status))
        {
            memcpy(output, &result, sizeof(result));
            information = sizeof(result);
        }
        return status;
    }
    case IOCTL_MTT_REMOVE_MONITOR:
        return RemoveMonitor(device, remove.Id);
    case IOCTL_MTT_CLEAR_MONITORS:
        return ClearMonitors(device);
    default:
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

NTSTATUS StartAdapter(DeviceContext* device) noexcept
{
    InterlockedExchange(&device->AdapterFinished, 0);
    InterlockedExchange(&device->AdapterGone, 0);
    device->AdapterStatus = STATUS_DEVICE_NOT_READY;
    IDDCX_ENDPOINT_VERSION version = {};
    version.Size = sizeof(version);
    version.MajorVer = 1;
    IDDCX_ADAPTER_CAPS caps = {};
    caps.Size = sizeof(caps);
    caps.MaxMonitorsSupported = MTT_MAX_MONITORS;
    caps.EndPointDiagnostics.Size = sizeof(caps.EndPointDiagnostics);
    caps.EndPointDiagnostics.GammaSupport = IDDCX_FEATURE_IMPLEMENTATION_NONE;
    caps.EndPointDiagnostics.TransmissionType = IDDCX_TRANSMISSION_TYPE_WIRED_OTHER;
    caps.EndPointDiagnostics.pEndPointFriendlyName = L"Monitor Topology Test";
    caps.EndPointDiagnostics.pEndPointManufacturerName = L"Topology Test";
    caps.EndPointDiagnostics.pEndPointModelName = L"Session-owned virtual adapter";
    caps.EndPointDiagnostics.pFirmwareVersion = &version;
    caps.EndPointDiagnostics.pHardwareVersion = &version;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, AdapterContext);
    attributes.EvtCleanupCallback = TopologyAdapterCleanup;
    attributes.SynchronizationScope = WdfSynchronizationScopeNone;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    IDARG_IN_ADAPTER_INIT input = {};
    input.WdfDevice = device->Device;
    input.pCaps = &caps;
    input.ObjectAttributes = &attributes;
    IDARG_OUT_ADAPTER_INIT output = {};
    NTSTATUS status = IddCxAdapterInitAsync(&input, &output);
    if (NT_SUCCESS(status))
    {
        device->Adapter = output.AdapterObject;
        auto* adapter = GetAdapterContext(output.AdapterObject);
        ExclusiveLock lock(adapter->Lock);
        // Publish the device link and any completion that raced the return
        // from InitAsync in one handshake. Later callbacks never take Gate.
        adapter->Device = device;
        if (adapter->Finished)
        {
            device->AdapterStatus = adapter->InitStatus;
            InterlockedExchange(&device->AdapterFinished, 1);
        }
    }
    else
    {
        LogFailure(L"IddCxAdapterInitAsync", status);
    }
    return status;
}
}

SwapChainWorker::SwapChainWorker(IDDCX_SWAPCHAIN chain, LUID renderAdapter,
    HANDLE frameEvent) noexcept :
    m_chain(chain), m_renderAdapter(renderAdapter), m_frameEvent(frameEvent)
{
}

SwapChainWorker::~SwapChainWorker() noexcept
{
    Stop();
    if (m_thread) CloseHandle(m_thread);
    if (m_stopEvent) CloseHandle(m_stopEvent);
    if (m_startEvent) CloseHandle(m_startEvent);
}

HRESULT SwapChainWorker::Prepare() noexcept
{
    m_startEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_startEvent) return HRESULT_FROM_WIN32(GetLastError());
    m_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent) return HRESULT_FROM_WIN32(GetLastError());
    m_thread = CreateThread(nullptr, 0, ThreadEntry, this, 0, nullptr);
    if (!m_thread) return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

void SwapChainWorker::Accept() noexcept
{
    m_ownsChain = true;
    SetEvent(m_startEvent);
}

void SwapChainWorker::Stop() noexcept
{
    if (m_stopEvent) SetEvent(m_stopEvent);
    if (m_startEvent) SetEvent(m_startEvent);
    if (m_thread) WaitForSingleObject(m_thread, INFINITE);
}

DWORD WINAPI SwapChainWorker::ThreadEntry(void* argument) noexcept
{
    auto* self = static_cast<SwapChainWorker*>(argument);
    WaitForSingleObject(self->m_startEvent, INFINITE);
    if (self->m_ownsChain)
    {
        const HRESULT hr = self->Run();
        if (FAILED(hr))
        {
            LogFailure(L"swapchain processing (abandoning for OS retry)", hr);
        }
        // Ownership follows successful AssignSwapChain, including GPU loss.
        // No monitor lock is held, and the context never waits on itself.
        WdfObjectDelete(self->m_chain);
        self->m_chain = nullptr;
    }
    return 0;
}

HRESULT SwapChainWorker::Run() noexcept
{
    if (WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0) return S_OK;
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return hr;
    ComPtr<IDXGIAdapter1> adapter;
    hr = factory->EnumAdapterByLuid(m_renderAdapter, IID_PPV_ARGS(&adapter));
    if (FAILED(hr)) return hr;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
        &device, nullptr, &context);
    if (FAILED(hr)) return hr;
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device.As(&dxgiDevice);
    if (FAILED(hr)) return hr;
    IDARG_IN_SWAPCHAINSETDEVICE setDevice = {};
    setDevice.pDevice = dxgiDevice.Get();
    hr = IddCxSwapChainSetDevice(m_chain, &setDevice);
    if (FAILED(hr)) return hr;

    const HANDLE events[] = { m_stopEvent, m_frameEvent };
    for (;;)
    {
        // Check even when frames arrive continuously; teardown cannot starve.
        if (WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0) return S_OK;
        hr = device->GetDeviceRemovedReason();
        if (FAILED(hr)) return hr;
        IDARG_OUT_RELEASEANDACQUIREBUFFER buffer = {};
        hr = IddCxSwapChainReleaseAndAcquireBuffer(m_chain, &buffer);
        if (hr == E_PENDING)
        {
            const DWORD wait = WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE, 1000);
            if (wait == WAIT_OBJECT_0) return S_OK;
            if (wait == WAIT_OBJECT_0 + 1 || wait == WAIT_TIMEOUT) continue;
            return wait == WAIT_FAILED ? HRESULT_FROM_WIN32(GetLastError()) : E_UNEXPECTED;
        }
        if (FAILED(hr)) return hr;
        ComPtr<IDXGIResource> surface;
        surface.Attach(buffer.MetaData.pSurface);
        // Deliberately discard, never encode, capture, persist, or transmit.
        // Each successful acquire contributes a COM reference to release.
        surface.Reset();
        hr = IddCxSwapChainFinishedProcessingFrame(m_chain);
        if (FAILED(hr)) return hr;
    }
}

_Use_decl_annotations_
NTSTATUS TopologyAssignSwapChain(IDDCX_MONITOR object, const IDARG_IN_SETSWAPCHAIN* input)
{
    MonitorReference reference(object);
    auto* monitor = reference.Get();
    if (!monitor) return STATUS_GRAPHICS_INDIRECT_DISPLAY_ABANDON_SWAPCHAIN;
    AcquireSRWLockExclusive(&monitor->Lock);
    if (monitor->Closing)
    {
        ReleaseSRWLockExclusive(&monitor->Lock);
        return STATUS_GRAPHICS_INDIRECT_DISPLAY_ABANDON_SWAPCHAIN;
    }
    const uint64_t epoch = ++monitor->Epoch;
    ++monitor->Assignments;
    auto* oldWorker = monitor->Worker;
    monitor->Worker = nullptr;
    ReleaseSRWLockExclusive(&monitor->Lock);
    delete oldWorker;

    auto* worker = new (std::nothrow) SwapChainWorker(input->hSwapChain,
        input->RenderAdapterLuid, input->hNextSurfaceAvailable);
    const HRESULT hr = worker ? worker->Prepare() : E_OUTOFMEMORY;
    if (FAILED(hr)) LogFailure(L"creating swapchain worker", hr);

    AcquireSRWLockExclusive(&monitor->Lock);
    const bool accepted = SUCCEEDED(hr) && !monitor->Closing && monitor->Epoch == epoch;
    if (accepted)
    {
        monitor->Worker = worker;
        worker->Accept();
    }
    ReleaseSRWLockExclusive(&monitor->Lock);
    if (!accepted)
    {
        // Returning failure leaves swapchain deletion ownership with IddCx.
        delete worker;
    }
    AcquireSRWLockExclusive(&monitor->Lock);
    --monitor->Assignments;
    WakeAllConditionVariable(&monitor->Idle);
    ReleaseSRWLockExclusive(&monitor->Lock);
    return accepted ? STATUS_SUCCESS : STATUS_GRAPHICS_INDIRECT_DISPLAY_ABANDON_SWAPCHAIN;
}

_Use_decl_annotations_
NTSTATUS TopologyUnassignSwapChain(IDDCX_MONITOR object)
{
    MonitorReference reference(object);
    if (reference.Get()) StopMonitor(reference.Get(), false);
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void TopologyMonitorCleanup(WDFOBJECT object)
{
    auto* context = GetMonitorContext(object);
    MonitorState* monitor;
    {
        ExclusiveLock lock(context->Lock);
        monitor = context->State;
        context->State = nullptr;
        if (monitor) InterlockedExchange(&monitor->Gone, 1);
    }
    if (monitor)
    {
        StopMonitor(monitor, true);
        ReleaseMonitorState(monitor);
    }
}

_Use_decl_annotations_
void TopologyAdapterCleanup(WDFOBJECT object)
{
    auto* adapter = GetAdapterContext(object);
    ExclusiveLock lock(adapter->Lock);
    if (adapter->Device)
    {
        // A WDF child's cleanup runs before its parent's cleanup.
        InterlockedExchange(&adapter->Device->AdapterGone, 1);
        adapter->Device = nullptr;
    }
}

_Use_decl_annotations_
NTSTATUS TopologyAdapterInitFinished(IDDCX_ADAPTER object, const IDARG_IN_ADAPTER_INIT_FINISHED* input)
{
    auto* adapter = GetAdapterContext(object);
    ExclusiveLock lock(adapter->Lock);
    adapter->InitStatus = input->AdapterInitStatus;
    adapter->Finished = true;
    if (adapter->Device)
    {
        adapter->Device->AdapterStatus = input->AdapterInitStatus;
        InterlockedExchange(&adapter->Device->AdapterFinished, 1);
    }
    if (!NT_SUCCESS(input->AdapterInitStatus))
    {
        LogFailure(L"adapter initialization", input->AdapterInitStatus);
    }
    // No monitors are created here.
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS TopologyCommitModes(IDDCX_ADAPTER object, const IDARG_IN_COMMITMODES* input)
{
    UNREFERENCED_PARAMETER(object);
    UNREFERENCED_PARAMETER(input);
    // There is no physical scan-out to configure. IddCx manages assignments.
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS TopologyParseDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* input,
    IDARG_OUT_PARSEMONITORDESCRIPTION* output)
{
    UNREFERENCED_PARAMETER(input);
    output->MonitorModeBufferOutputCount = 0;
    output->PreferredMonitorModeIdx = 0;
    // All our monitors use the documented no-description callback instead.
    return STATUS_NOT_SUPPORTED;
}

_Use_decl_annotations_
NTSTATUS TopologyDefaultModes(IDDCX_MONITOR object, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* input,
    IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* output)
{
    output->DefaultMonitorModeBufferOutputCount = 1;
    output->PreferredMonitorModeIdx = 0;
    if (input->DefaultMonitorModeBufferInputCount == 0) return STATUS_SUCCESS;
    if (!input->pDefaultMonitorModes) return STATUS_INVALID_PARAMETER;
    MonitorReference reference(object);
    if (!reference.Get()) return STATUS_DEVICE_REMOVED;
    IDDCX_MONITOR_MODE mode = {};
    mode.Size = sizeof(mode);
    mode.Origin = IDDCX_MONITOR_MODE_ORIGIN_DRIVER;
    mode.MonitorVideoSignalInfo = reference.Get()->Signal;
    input->pDefaultMonitorModes[0] = mode;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS TopologyTargetModes(IDDCX_MONITOR object, const IDARG_IN_QUERYTARGETMODES* input,
    IDARG_OUT_QUERYTARGETMODES* output)
{
    output->TargetModeBufferOutputCount = 1;
    if (input->TargetModeBufferInputCount == 0) return STATUS_SUCCESS;
    if (!input->pTargetModes) return STATUS_INVALID_PARAMETER;
    MonitorReference reference(object);
    if (!reference.Get()) return STATUS_DEVICE_REMOVED;
    IDDCX_TARGET_MODE mode = {};
    mode.Size = sizeof(mode);
    mode.TargetVideoSignalInfo.targetVideoSignalInfo = reference.Get()->Signal;
    // IddCx target-mode contract overrides the generic DISPLAYCONFIG advice:
    // desktop update rate = vSyncFreq / divider, which must be nonzero.
    mode.TargetVideoSignalInfo.targetVideoSignalInfo.AdditionalSignalInfo.vSyncFreqDivider = 1;
    input->pTargetModes[0] = mode;
    return STATUS_SUCCESS;
}

_Use_decl_annotations_
void TopologyIoControl(WDFDEVICE device, WDFREQUEST request, size_t outputLength,
    size_t inputLength, ULONG code)
{
    ULONG_PTR information = 0;
    const NTSTATUS status = ControlRequest(GetDeviceContext(device), request,
        outputLength, inputLength, code, information);
    if (!NT_SUCCESS(status)) LogFailure(L"control request", status);
    WdfRequestCompleteWithInformation(request, status, information);
}

_Use_decl_annotations_
void TopologyFileCreate(WDFDEVICE object, WDFREQUEST request, WDFFILEOBJECT file)
{
    LogMessage(L"MonitorTopologyTestDriver: controller file-create callback entered.\n");
    auto* device = GetDeviceContext(object);
    NTSTATUS status = STATUS_SUCCESS;
    {
        ExclusiveLock lock(device->Gate);
        ReapDestroyedMonitors(device);
        const auto name = WdfFileObjectGetFileName(file);
        if (name && name->Length)
        {
            wchar_t message[192] = {};
            const int length = static_cast<int>(name->Length / sizeof(wchar_t));
            if (SUCCEEDED(StringCchPrintfW(message, ARRAYSIZE(message),
                L"MonitorTopologyTestDriver: file-create name '%.*s'.\n",
                length < 128 ? length : 128, name->Buffer)))
            {
                LogMessage(message);
            }
        }
        if (device->Removing) status = STATUS_DELETE_PENDING;
        else if (!name || name->Length == 0)
        {
            // IddCx and the graphics stack open the base adapter themselves.
            // These opens neither acquire a controller session nor authorize
            // our private IOCTLs, which require Owner and IsController.
            LogMessage(L"MonitorTopologyTestDriver: base-adapter open, no controller ownership.\n");
        }
        else if (!IsControlFileName(name))
        {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        else if (device->Owner || device->Orphaned)
        {
            status = STATUS_DEVICE_BUSY;
        }
        else
        {
            device->Owner = file;
            GetFileContext(file)->IsController = true;
            LogMessage(L"MonitorTopologyTestDriver: named controller session accepted.\n");
        }
    }
    if (!NT_SUCCESS(status)) LogFailure(L"opening controller session", status);
    WdfRequestComplete(request, status);
}

_Use_decl_annotations_
void TopologyFileCleanup(WDFFILEOBJECT file)
{
    auto* device = GetDeviceContext(WdfFileObjectGetDevice(file));
    ExclusiveLock lock(device->Gate);
    if (device->Owner == file && GetFileContext(file)->IsController)
    {
        // Cleanup covers a closed handle or terminated owner, not a hung
        // controller which keeps its handle open. V1 has no heartbeat/lease.
        const NTSTATUS status = ClearMonitors(device);
        GetFileContext(file)->IsController = false;
        device->Owner = nullptr;
        // Cleanup cannot return an NTSTATUS. Preserve residual slots, log the
        // failure, and block new sessions until actual destruction/restart.
        device->Orphaned = !NT_SUCCESS(status) || MonitorCount(device) != 0;
        if (device->Orphaned) LogFailure(L"file cleanup (session quarantined)", status);
    }
}

_Use_decl_annotations_
NTSTATUS TopologyD0Entry(WDFDEVICE object, WDF_POWER_DEVICE_STATE previous)
{
    UNREFERENCED_PARAMETER(previous);
    auto* device = GetDeviceContext(object);
    ExclusiveLock lock(device->Gate);
    if (device->Removing) return STATUS_DELETE_PENDING;
    ReapDestroyedMonitors(device);
    if (device->Adapter && ReadAtomic(&device->AdapterGone))
    {
        device->Adapter = nullptr;
    }
    // Retry failed departures before accepting any new topology mutation.
    const NTSTATUS clearStatus = ClearMonitors(device);
    if (!NT_SUCCESS(clearStatus)) return clearStatus;
    device->Orphaned = false;
    NTSTATUS status = STATUS_SUCCESS;
    if (!device->Adapter) status = StartAdapter(device);
    device->Powered = NT_SUCCESS(status);
    ++device->Generation;
    return status;
}

_Use_decl_annotations_
NTSTATUS TopologyD0Exit(WDFDEVICE object, WDF_POWER_DEVICE_STATE target)
{
    UNREFERENCED_PARAMETER(target);
    auto* device = GetDeviceContext(object);
    ExclusiveLock lock(device->Gate);
    device->Powered = false;
    ++device->Generation;
    const NTSTATUS status = ClearMonitors(device);
    if (!NT_SUCCESS(status))
    {
        device->Orphaned = true;
        LogFailure(L"power-down cleanup (device not reusable)", status);
    }
    return status;
}

_Use_decl_annotations_
void TopologyDeviceCleanup(WDFOBJECT object)
{
    auto* device = GetDeviceContext(object);
    ExclusiveLock lock(device->Gate);
    device->Removing = true;
    device->Powered = false;
    // IddCx owns the adapter and child teardown on device removal. Release
    // only C++ state here; WDF contexts must not be accessed after cleanup.
    for (auto& slot : device->Slots)
    {
        if (slot.Object)
        {
            StopMonitor(slot.State, true);
            ReleaseMonitorState(slot.State);
            slot = {};
        }
    }
    if (device->Adapter)
    {
        device->Adapter = nullptr;
    }
    device->Owner = nullptr;
}

_Use_decl_annotations_
NTSTATUS TopologyDeviceAdd(WDFDRIVER driver, PWDFDEVICE_INIT init)
{
    UNREFERENCED_PARAMETER(driver);
    WDF_PNPPOWER_EVENT_CALLBACKS power;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power);
    power.EvtDeviceD0Entry = TopologyD0Entry;
    power.EvtDeviceD0Exit = TopologyD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(init, &power);
    WdfDeviceInitSetIoType(init, WdfDeviceIoBuffered);
    // UMDF does not expose the KMDF-only SetDeviceType/SetCharacteristics/
    // SetExclusive APIs. The INF sets type and secure-open properties, while
    // controller exclusivity is enforced only on the named control endpoint.

    WDF_FILEOBJECT_CONFIG files;
    WDF_FILEOBJECT_CONFIG_INIT(&files, TopologyFileCreate, WDF_NO_EVENT_CALLBACK, TopologyFileCleanup);
    files.AutoForwardCleanupClose = WdfFalse;
    files.FileObjectClass = WdfFileObjectWdfCannotUseFsContexts;
    WDF_OBJECT_ATTRIBUTES fileAttributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fileAttributes, FileContext);
    fileAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
    fileAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    WdfDeviceInitSetFileObjectConfig(init, &files, &fileAttributes);

    IDD_CX_CLIENT_CONFIG config;
    IDD_CX_CLIENT_CONFIG_INIT(&config);
    config.EvtIddCxDeviceIoControl = TopologyIoControl;
    config.EvtIddCxAdapterInitFinished = TopologyAdapterInitFinished;
    config.EvtIddCxAdapterCommitModes = TopologyCommitModes;
    config.EvtIddCxParseMonitorDescription = TopologyParseDescription;
    config.EvtIddCxMonitorGetDefaultDescriptionModes = TopologyDefaultModes;
    config.EvtIddCxMonitorQueryTargetModes = TopologyTargetModes;
    config.EvtIddCxMonitorAssignSwapChain = TopologyAssignSwapChain;
    config.EvtIddCxMonitorUnassignSwapChain = TopologyUnassignSwapChain;
    NTSTATUS status = IddCxDeviceInitConfig(init, &config);
    if (!NT_SUCCESS(status)) return status;

    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DeviceContext);
    attributes.EvtCleanupCallback = TopologyDeviceCleanup;
    attributes.SynchronizationScope = WdfSynchronizationScopeNone;
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    WDFDEVICE device = nullptr;
    status = WdfDeviceCreate(&init, &attributes, &device);
    if (!NT_SUCCESS(status)) return status;
    GetDeviceContext(device)->Device = device;
    // Register the companion interface before IddCx initializes the device.
    UNICODE_STRING reference = {};
    reference.Buffer = const_cast<PWSTR>(ControlReference);
    reference.Length = sizeof(ControlReference) - sizeof(wchar_t);
    reference.MaximumLength = sizeof(ControlReference);
    status = WdfDeviceCreateDeviceInterface(device, &MTT_DEVICE_INTERFACE, &reference);
    if (!NT_SUCCESS(status))
    {
        LogFailure(L"creating control interface", status);
        return status;
    }
    status = IddCxDeviceInitialize(device);
    return status;
}

_Use_decl_annotations_
void TopologyDriverCleanup(WDFOBJECT object)
{
    UNREFERENCED_PARAMETER(object);
    if (TopologyTraceHandle)
    {
        EventUnregister(TopologyTraceHandle);
        TopologyTraceHandle = 0;
    }
}

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
    EventRegister(&TopologyTraceProvider, nullptr, nullptr, &TopologyTraceHandle);
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, TopologyDeviceAdd);
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = TopologyDriverCleanup;
    const NTSTATUS status = WdfDriverCreate(driver, registry, &attributes, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) TopologyDriverCleanup(nullptr);
    return status;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(reason);
    UNREFERENCED_PARAMETER(reserved);
    return TRUE;
}
