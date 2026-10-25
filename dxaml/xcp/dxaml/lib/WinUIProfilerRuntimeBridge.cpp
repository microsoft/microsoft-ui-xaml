// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "WinUIProfilerTracing.h"
#include <evntprov.h>
#include <evntrace.h>
#include "WinUIProfilerEventEncoder.h"
#include <new>
#include <cmath>
#include <cstring>
#include <type_traits>
#include "WinUIProfilerHooks.h"
#include "WinUIProfilerGraphicsHooks.h"
#include "WinUIProfilerResourceHooks.h"
#include "WinUIProfilerObserverSuppression.h"
#include "WinUIProfilerAllocatorRuntime.h"
#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <weakreference.h>
#include <wrl/client.h>

namespace WinUIProfiler
{
    namespace
    {
        INIT_ONCE s_identityOnce = INIT_ONCE_STATIC_INIT;
        ProtocolInfo s_info{};
        HRESULT s_identityResult = E_UNEXPECTED;
        SRWLOCK s_stateLock = SRWLOCK_INIT;
        SequenceDomain s_domains[17];
        uint64_t s_objectId = 0;
        int64_t s_epochGeneration = 0;
        volatile LONG64 s_enableGeneration = 0;
        volatile LONG64 s_capabilitiesSerial = 0;
        int64_t s_publishedSerial = -1;
        thread_local bool s_insideInstrumentation = false;
        thread_local bool s_observerBeforeInstrumentation = false;
        thread_local bool s_resolvingEligibility = false;
        SRWLOCK s_eligibilityLock = SRWLOCK_INIT;
        volatile LONG64 s_cachedEligibilitySerial = -1;
        volatile LONG64 s_enabledDefinitions = 0;
        volatile LONG64 s_fastDefinitions = 0;
        volatile LONG64 s_identityGeneration = 0;
        volatile LONG64 s_lifetimeGeneration = 0;
        volatile LONG64 s_frameGeneration = 0;
        volatile LONG64 s_slotGeneration = 0;
        volatile LONG64 s_correlationGeneration = 0;
        volatile LONG64 s_treeGeneration = 0;
        volatile LONG64 s_allocatorGeneration = 1;
        volatile LONG64 s_resourceGeneration = 0;
        // Protected by s_stateLock. Selection generation is only an invalidation
        // token; the monotonic mapping generation also advances on producer renewal.
        GUID s_allocatorEpoch{};
        GUID s_allocatorProducerEpoch{};
        LONG64 s_allocatorMappedSelection = 0;
        uint64_t s_allocatorContextGeneration = 0;
        uint64_t s_allocatorReportedLoss = 0;
        uint64_t s_allocatorQueuedRegistryLoss = 0; // Serialized by the allocator transaction.
        constexpr uint16_t AllocatorOriginCapacity = 64;
        struct AllocatorOrigin
        {
            GUID Epoch{};
            volatile LONG References = 0;
            uint64_t LastLossSequence = 0;
            bool Retired = false;
        };
        AllocatorOrigin s_allocatorOrigins[AllocatorOriginCapacity]{};
        uint16_t s_allocatorCurrentOrigin = 0; // Protected by s_stateLock.
        SRWLOCK s_allocatorLossLock = SRWLOCK_INIT;
        struct PendingAllocatorLoss
        {
            int64_t StartQpc = 0;
            uint64_t CoverageGaps = 0;
        };
        PendingAllocatorLoss s_allocatorPendingLoss[AllocatorOriginCapacity]{};
        volatile LONG s_allocatorPendingOrigins = 0;
#ifdef WINUI_PROFILER_STANDALONE
        unsigned s_allocatorContextRaceForTest = 0;
        bool s_allocatorFailLossWriteForTest = false;
#endif
        constexpr uint64_t SlotObservationMask = (uint64_t{1} << 2) | (uint64_t{1} << 5) |
            (uint64_t{1} << 6) | (uint64_t{1} << 7) | (uint64_t{1} << 14);

        SRWLOCK s_listenerLock = SRWLOCK_INIT;
        bool s_anyListener = false;
        volatile LONG s_providerFault = 0;
        REGHANDLE s_providerHandle = 0;
        constexpr char ProviderName[] = "WinUIProfiler-Runtime";
        BYTE s_providerTraits[sizeof(ProviderName) + 2]{};
        uint64_t EventKeyword(Event event) noexcept;
        ULONG QueryDefinitionSelections(uint64_t* mask, BYTE* buffer, ULONG bufferSize) noexcept;
        // Serialized by s_listenerLock; callbacks never allocate eligibility scratch.
        alignas(TRACE_ENABLE_INFO) BYTE s_notificationEligibilityBuffer[64 * 1024];

        void NTAPI OnEnable(LPCGUID, ULONG control, UCHAR, ULONGLONG, ULONGLONG,
            PEVENT_FILTER_DESCRIPTOR, PVOID) noexcept
        {
            ObserverSuppression::Scope observer;
            // Notification only: no COM, GUID creation, allocation or dispatcher work.
            AcquireSRWLockExclusive(&s_listenerLock);
            if (control == EVENT_CONTROL_CODE_ENABLE_PROVIDER)
            {
                // Notification arguments can combine different listeners' filters.
                // Query the actual enabled definitions, without constructing receipt proof.
                if (!s_anyListener)
                {
                    s_anyListener = true;
                    InterlockedIncrement64(&s_enableGeneration);
                }
                uint64_t mask = 0;
                const ULONG selectionError = QueryDefinitionSelections(&mask,
                    s_notificationEligibilityBuffer, sizeof(s_notificationEligibilityBuffer));
                if (selectionError)
                {
                    mask = 0;
                    InterlockedExchange(&s_providerFault, selectionError);
                }
                constexpr uint64_t identityMask = (uint64_t{1} << 2) | (uint64_t{1} << 5) |
                    (uint64_t{1} << 6) | (uint64_t{1} << 11) | (uint64_t{1} << 14);
                const uint64_t previous = static_cast<uint64_t>(InterlockedExchange64(&s_fastDefinitions, static_cast<LONG64>(mask)));
                if ((previous ^ mask) & (uint64_t{1} << 12)) InterlockedIncrement64(&s_allocatorGeneration);
                if ((previous & (uint64_t{1} << 11)) && !(mask & (uint64_t{1} << 11)))
                    InterlockedIncrement64(&s_resourceGeneration);
                if ((previous & identityMask) && !(mask & identityMask)) InterlockedIncrement64(&s_identityGeneration);
                if ((previous & SlotObservationMask) && !(mask & SlotObservationMask)) InterlockedIncrement64(&s_slotGeneration);
                if ((previous & (uint64_t{1} << 7)) && !(mask & (uint64_t{1} << 7))) InterlockedIncrement64(&s_correlationGeneration);
                if ((previous & (uint64_t{1} << 5)) && !(mask & (uint64_t{1} << 5))) InterlockedIncrement64(&s_treeGeneration);
                if ((previous & (uint64_t{1} << 2)) && !(mask & (uint64_t{1} << 2))) InterlockedIncrement64(&s_lifetimeGeneration);
                if ((previous & (uint64_t{1} << 9)) && !(mask & (uint64_t{1} << 9))) InterlockedIncrement64(&s_frameGeneration);
            }
            else if (control == EVENT_CONTROL_CODE_DISABLE_PROVIDER)
            {
                s_anyListener = false;
                InterlockedExchange64(&s_fastDefinitions, 0);
                InterlockedIncrement64(&s_allocatorGeneration);
                InterlockedIncrement64(&s_resourceGeneration);
                InterlockedIncrement64(&s_identityGeneration);
                InterlockedIncrement64(&s_lifetimeGeneration);
                InterlockedIncrement64(&s_frameGeneration);
                InterlockedIncrement64(&s_slotGeneration);
                InterlockedIncrement64(&s_correlationGeneration);
                InterlockedIncrement64(&s_treeGeneration);
            }
            InterlockedIncrement64(&s_capabilitiesSerial);
            ReleaseSRWLockExclusive(&s_listenerLock);
        }

        void* ObserveAllocate(HANDLE heap, DWORD flags, SIZE_T requested) noexcept
        {
            AllocatorObservation::AllocatorRuntime runtime;
            return AllocatorObservation::HeapAdapter<AllocatorObservation::AllocatorRuntime>(runtime).Allocate(heap, flags, requested);
        }
        void* ObserveResize(HANDLE heap, DWORD flags, void* address, SIZE_T requested) noexcept
        {
            AllocatorObservation::AllocatorRuntime runtime;
            return AllocatorObservation::HeapAdapter<AllocatorObservation::AllocatorRuntime>(runtime).Resize(heap, flags, address, requested);
        }
        BOOL ObserveFree(HANDLE heap, DWORD flags, void* address) noexcept
        {
            AllocatorObservation::AllocatorRuntime runtime;
            return AllocatorObservation::HeapAdapter<AllocatorObservation::AllocatorRuntime>(runtime).Free(heap, flags, address);
        }
        constexpr AllocatorObservation::HeapObservers s_heapObservers{ObserveAllocate, ObserveResize, ObserveFree};

        struct Registration
        {
            Registration() noexcept
            {
                ObserverSuppression::Scope observer;
                s_providerTraits[0] = static_cast<BYTE>(sizeof(s_providerTraits));
                s_providerTraits[1] = static_cast<BYTE>(sizeof(s_providerTraits) >> 8);
                memcpy(s_providerTraits + 2, ProviderName, sizeof(ProviderName));
                ULONG result = EventRegister(&RuntimeProviderId, OnEnable, nullptr, &s_providerHandle);
                if (result == ERROR_SUCCESS)
                    result = EventSetInformation(s_providerHandle, EventProviderSetTraits, s_providerTraits, sizeof(s_providerTraits));
                if (result != ERROR_SUCCESS) InterlockedExchange(&s_providerFault, result);
                else AllocatorObservation::SetHeapObservers(&s_heapObservers);
            }
            ~Registration()
            {
                ObserverSuppression::Scope observer;
                AllocatorObservation::SetHeapObservers(nullptr);
                if (s_providerHandle) EventUnregister(s_providerHandle);
            }
        } s_registration;

        uint64_t EventKeyword(Event event) noexcept
        {
            switch (event)
            {
            case Event::RuntimeCapabilities: case Event::Loss: return 1;
            case Event::ObjectLifetime: case Event::TypeResolved: case Event::CorePeerAssociation: case Event::XamlPropertyChanged: return 2;
            case Event::TreeEdge: return 4;
            case Event::VisualDelta: case Event::Correlation: return 8;
            case Event::Boundary: case Event::Frame: return 16;
            case Event::Device: case Event::Resource: return 32;
            case Event::Allocation: return 64;
            case Event::CaptureBoundary: case Event::CaptureChunk: return 128;
            default: return 0;
            }

        }

        ULONG QueryDefinitionSelections(uint64_t* mask, BYTE* buffer, ULONG bufferSize) noexcept
        {
            ULONG bytes = 0;
            GUID provider = RuntimeProviderId;
            const ULONG status = EnumerateTraceGuidsEx(TraceGuidQueryInfo, &provider, sizeof(provider),
                buffer, bufferSize, &bytes);
            if (status != ERROR_SUCCESS) return status;
            if (bytes < sizeof(TRACE_GUID_INFO) || bytes > bufferSize) return ERROR_INVALID_DATA;
            const auto info = reinterpret_cast<const TRACE_GUID_INFO*>(buffer);
            size_t offset = sizeof(TRACE_GUID_INFO);
            for (ULONG i = 0; i < info->InstanceCount; ++i)
            {
                if (offset > bytes || bytes - offset < sizeof(TRACE_PROVIDER_INSTANCE_INFO)) return ERROR_INVALID_DATA;
                const auto instance = reinterpret_cast<const TRACE_PROVIDER_INSTANCE_INFO*>(buffer + offset);
                if (instance->EnableCount > (bytes - offset - sizeof(*instance)) / sizeof(TRACE_ENABLE_INFO)) return ERROR_INVALID_DATA;
                const auto enables = reinterpret_cast<const TRACE_ENABLE_INFO*>(instance + 1);
                if (instance->Pid == GetCurrentProcessId())
                {
                    for (ULONG j = 0; j < instance->EnableCount; ++j)
                    {
                        for (uint16_t id = 1; id <= 16; ++id)
                        {
                            const auto definition = static_cast<Event>(id);
                            const uint64_t keyword = EventKeyword(definition);
                            const UCHAR level = definition == Event::Loss ? 3 : 4;
                            if (keyword && enables[j].IsEnabled && (enables[j].Level == 0 || enables[j].Level >= level) &&
                                (keyword & enables[j].MatchAnyKeyword) != 0 &&
                                (keyword & enables[j].MatchAllKeyword) == enables[j].MatchAllKeyword)
                                *mask |= uint64_t{1} << id;
                        }
                    }
                }
                if (i + 1 < info->InstanceCount && instance->NextOffset < sizeof(*instance) + instance->EnableCount * sizeof(TRACE_ENABLE_INFO))
                    return ERROR_INVALID_DATA;
                offset += instance->NextOffset;
            }
            return ERROR_SUCCESS;
        }

        ULONG QueryDefinitionSelections(uint64_t* mask) noexcept
        {
            constexpr ULONG bufferSize = 64 * 1024;
            struct Scratch
            {
                BYTE* Data;
                ~Scratch() { if (Data) HeapFree(GetProcessHeap(), 0, Data); }
            } scratch{static_cast<BYTE*>(HeapAlloc(GetProcessHeap(), 0, bufferSize))};
            if (!scratch.Data) return ERROR_NOT_ENOUGH_MEMORY;
            return QueryDefinitionSelections(mask, scratch.Data, bufferSize);
        }

        bool DefinitionEnabled(Event event) noexcept
        {
            ObserverSuppression::Scope observer;
            if (s_resolvingEligibility) return false;
            const int64_t serial = InterlockedCompareExchange64(&s_capabilitiesSerial, 0, 0);
            if (serial != InterlockedCompareExchange64(&s_cachedEligibilitySerial, 0, 0))
            {
                s_resolvingEligibility = true;
                AcquireSRWLockExclusive(&s_eligibilityLock);
                const int64_t current = InterlockedCompareExchange64(&s_capabilitiesSerial, 0, 0);
                if (current != InterlockedCompareExchange64(&s_cachedEligibilitySerial, 0, 0))
                {
                    uint64_t mask = 0;
                    const ULONG error = QueryDefinitionSelections(&mask);
                    if (error)
                    {
                        mask = 0;
                        InterlockedExchange(&s_providerFault, error);
                    }
                    // Cache only definition eligibility, never listener proof.
                    if (current == InterlockedCompareExchange64(&s_capabilitiesSerial, 0, 0))
                    {
                        InterlockedExchange64(&s_enabledDefinitions, static_cast<LONG64>(mask));
                        InterlockedExchange64(&s_cachedEligibilitySerial, current);
                    }
                }
                ReleaseSRWLockExclusive(&s_eligibilityLock);
                s_resolvingEligibility = false;
            }
            return InterlockedCompareExchange64(&s_cachedEligibilitySerial, 0, 0) ==
                InterlockedCompareExchange64(&s_capabilitiesSerial, 0, 0) &&
                (static_cast<uint64_t>(InterlockedCompareExchange64(&s_enabledDefinitions, 0, 0)) & (uint64_t{1} << static_cast<uint16_t>(event))) != 0;
        }

        BOOL CALLBACK InitializeIdentity(PINIT_ONCE, PVOID, PVOID*) noexcept
        {
            s_info.Size = sizeof(s_info);
            s_info.Version = ProtocolVersion;
            s_info.SupportedEvents = 0x1fffe; // All schema writers; not observational/capture capabilities.
            s_info.Process.ProcessId = GetCurrentProcessId();
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
            {
                s_info.Process.HasCreationTime = 1;
                s_info.Process.CreationTimeFileTime =
                    static_cast<int64_t>((static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime);
            }
            LARGE_INTEGER frequency{};
            QueryPerformanceFrequency(&frequency);
            s_info.QpcFrequency = frequency.QuadPart;
#if defined(_M_ARM64)
            s_info.Architecture = 1;
#else
            s_info.Architecture = 0;
#endif
            s_identityResult = CoCreateGuid(&s_info.Process.InstanceId);
            if (SUCCEEDED(s_identityResult)) s_identityResult = CoCreateGuid(&s_info.ProducerEpoch);
            return TRUE;
        }

        bool RetainAllocatorOrigin(uint16_t origin) noexcept
        {
            if (InterlockedIncrement(&s_allocatorOrigins[origin - 1].References) > 0) return true;
            InterlockedCompareExchange(&s_providerFault, ERROR_ARITHMETIC_OVERFLOW, 0);
            return false;
        }
        void ReleaseAllocatorOrigin(uint16_t origin) noexcept
        {
            if (origin) InterlockedDecrement(&s_allocatorOrigins[origin - 1].References);
        }
        uint16_t AcquireAllocatorOriginLocked() noexcept
        {
            if (!s_allocatorCurrentOrigin)
            {
                for (uint16_t i = 0; i != AllocatorOriginCapacity; ++i)
                {
                    auto& candidate = s_allocatorOrigins[i];
                    if (InterlockedCompareExchange(&candidate.References, 0, 0) != 0) continue;
                    candidate.Epoch = s_info.ProducerEpoch;
                    candidate.LastLossSequence = 0;
                    candidate.Retired = false;
                    s_allocatorCurrentOrigin = i + 1;
                    break;
                }
            }
            if (s_allocatorCurrentOrigin && RetainAllocatorOrigin(s_allocatorCurrentOrigin))
                return s_allocatorCurrentOrigin;
            InterlockedCompareExchange(&s_providerFault, ERROR_BUFFER_OVERFLOW, 0);
            return 0;
        }
        void QueueAllocatorLossLocked(uint16_t origin, int64_t startQpc, uint64_t gaps) noexcept
        {
            if (!origin || !gaps) return;
            auto& pending = s_allocatorPendingLoss[origin - 1];
            if (UINT64_MAX - pending.CoverageGaps < gaps)
            {
                InterlockedCompareExchange(&s_providerFault, ERROR_ARITHMETIC_OVERFLOW, 0);
                return;
            }
            if (!pending.CoverageGaps)
            {
                if (!RetainAllocatorOrigin(origin)) return;
                pending.StartQpc = startQpc;
                InterlockedIncrement(&s_allocatorPendingOrigins);
            }
            else if (startQpc < pending.StartQpc) pending.StartQpc = startQpc;
            pending.CoverageGaps += gaps;
        }
        HRESULT EnsureEpoch() noexcept
        {
            InitOnceExecuteOnce(&s_identityOnce, InitializeIdentity, nullptr, nullptr);
            if (FAILED(s_identityResult)) return s_identityResult;
            const LONG fault = InterlockedCompareExchange(&s_providerFault, 0, 0);
            if (fault) return HRESULT_FROM_WIN32(fault);
            AcquireSRWLockExclusive(&s_stateLock);
            const int64_t generation = InterlockedCompareExchange64(&s_enableGeneration, 0, 0);
            HRESULT result = S_OK;
            if (generation != s_epochGeneration)
            {
                GUID epoch;
                result = CoCreateGuid(&epoch);
                if (SUCCEEDED(result))
                {
                    if (s_allocatorCurrentOrigin)
                    {
                        auto& origin = s_allocatorOrigins[s_allocatorCurrentOrigin - 1];
                        uint64_t settled = 0;
                        s_domains[15].Snapshot(&origin.LastLossSequence, &settled);
                        origin.Retired = true;
                        s_allocatorCurrentOrigin = 0;
                    }
                    s_info.ProducerEpoch = epoch;
                    s_epochGeneration = generation;
                    s_objectId = 0;
                    s_publishedSerial = -1;
                    // Exclusive state lock excludes every outstanding emission.
                    for (auto& domain : s_domains)
                    {
                        domain.~SequenceDomain();
                        new (&domain) SequenceDomain;
                    }
                }
            }
            ReleaseSRWLockExclusive(&s_stateLock);
            return result;
        }

        template<typename Payload>
        HRESULT WriteNative(const Emission& emission, const Payload& payload, Event definition) noexcept
        {
            ObserverSuppression::Scope observer;
            if (emission.Definition != definition) return E_INVALIDARG;
            if (!IsEnabled(definition)) return HRESULT_FROM_WIN32(ERROR_CANCELLED);
            const HRESULT valid = Validate(payload);
            if (FAILED(valid)) return valid;
            static constexpr const char* names[] = {"", "RuntimeCapabilities", "ObjectLifetime", "TypeResolved",
                "CorePeerAssociation", "TreeEdge", "VisualDelta", "Correlation", "Boundary", "Frame", "Device",
                "Resource", "Allocation", "CaptureBoundary", "CaptureChunk", "Loss", "XamlPropertyChanged"};
            Wire::Encoder encoder(names[static_cast<uint16_t>(definition)]);
            encoder.U16("SemanticEventId", static_cast<uint16_t>(definition));
            encoder.U8("EventSchemaVersion", EventVersion);
            encoder.Guid("ProducerEpoch", emission.Info.ProducerEpoch);
            encoder.U64("SourceSequence", emission.SourceSequence);
            encoder.Struct("TargetProcess", 4);
            encoder.U32("ProcessId", emission.Info.Process.ProcessId);
            encoder.Bool("HasCreationTimeFileTime", emission.Info.Process.HasCreationTime != 0);
            encoder.I64("CreationTimeFileTime", emission.Info.Process.CreationTimeFileTime);
            encoder.Guid("InstanceId", emission.Info.Process.InstanceId);
            if constexpr (std::is_same_v<Payload, RuntimeCapabilities>)
            {
#ifndef WINUI_BUILD_VERSION
#define WINUI_BUILD_VERSION 0
#endif
#define WP_STRINGIFY_INNER(value) #value
#define WP_STRINGIFY(value) WP_STRINGIFY_INNER(value)
                encoder.Guid("Epoch", emission.Info.ProducerEpoch);
                encoder.Text("Build", "WinUI-" WP_STRINGIFY(WINUI_BUILD_VERSION));
                encoder.U32("Architecture", emission.Info.Architecture);
                encoder.I64("QpcFrequency", emission.Info.QpcFrequency);
            }
            if constexpr (std::is_same_v<Payload, CaptureChunk>)
            {
                const HRESULT encoded = Wire::EncodeCapture(encoder, payload);
                if (FAILED(encoded)) return encoded;
            }
            else Wire::Encode(encoder, payload);
            return encoder.Send(s_providerHandle, s_providerTraits, sizeof(s_providerTraits),
                EventKeyword(definition), definition == Event::Loss ? 3 : 4, emission.TimestampQpc);
        }

        HRESULT PublishCapabilities() noexcept
        {
            AcquireSRWLockExclusive(&s_stateLock);
            const int64_t serial = InterlockedCompareExchange64(&s_capabilitiesSerial, 0, 0);
            HRESULT result = S_OK;
            if (s_publishedSerial != serial && IsEnabled(Event::RuntimeCapabilities))
            {
                Emission emission;
                emission.Info = s_info;
                emission.Definition = Event::RuntimeCapabilities;
                emission.Sequence = &s_domains[1];
                result = emission.Sequence->Reserve(&emission.SourceSequence);
                if (SUCCEEDED(result))
                {
                    result = WriteNative(emission, RuntimeCapabilities{}, Event::RuntimeCapabilities);
                    emission.Sequence->Settle(emission.SourceSequence);
                    if (SUCCEEDED(result))
                    {
                        s_publishedSerial = serial;
                    }
                }
            }
            ReleaseSRWLockExclusive(&s_stateLock);
            return result;
        }
    }

    HRESULT GetProtocolInfo(uint32_t version, uint32_t size, ProtocolInfo* info) noexcept
    {
        if (!info) return E_POINTER;
        if (version != ProtocolVersion) return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        if (size != sizeof(*info)) return E_INVALIDARG;
        InstrumentationScope scope;
        *info = {};
        const HRESULT result = EnsureEpoch();
        if (FAILED(result)) return result;
        if (IsEnabled(Event::RuntimeCapabilities))
        {
            const HRESULT published = PublishCapabilities();
            if (FAILED(published)) return published;
        }
        const LONG fault = InterlockedCompareExchange(&s_providerFault, 0, 0);
        if (fault) return HRESULT_FROM_WIN32(fault);
        AcquireSRWLockShared(&s_stateLock);
        *info = s_info;
        ReleaseSRWLockShared(&s_stateLock);
        return S_OK;
    }
    HRESULT GetSequenceCut(uint32_t version, uint16_t eventId, uint32_t size, SequenceCut* cut) noexcept
    {
        if (!cut) return E_POINTER;
        if (version != ProtocolVersion) return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
        if (size != sizeof(*cut) || EventKeyword(static_cast<Event>(eventId)) == 0) return E_INVALIDARG;
        InstrumentationScope scope;
        *cut = {};
        HRESULT result = EnsureEpoch();
        if (FAILED(result)) return result;
        GUID cutId;
        result = CoCreateGuid(&cutId);
        if (FAILED(result)) return result;
        AcquireSRWLockShared(&s_stateLock);
        cut->Size = sizeof(*cut);
        cut->Version = ProtocolVersion;
        cut->CutId = cutId;
        cut->ProducerEpoch = s_info.ProducerEpoch;
        cut->Process = s_info.Process;
        cut->EventId = eventId;
        cut->EventSchemaVersion = EventVersion;
        s_domains[eventId].Snapshot(&cut->ReservedThrough, &cut->SettledThrough);
        ReleaseSRWLockShared(&s_stateLock);
        return S_OK;
    }
    HRESULT RequestCapture(uint32_t version, uint32_t, GUID) noexcept
    {
        return version == ProtocolVersion ? E_NOTIMPL : HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
    }

    HRESULT SequenceDomain::Reserve(uint64_t* sequence) noexcept
    {
        if (!sequence) return E_POINTER;
        *sequence = 0;
        AcquireSRWLockExclusive(&m_lock);
        HRESULT result = S_OK;
        if (m_reserved == UINT64_MAX) result = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        else if (m_reserved - m_settled == Window) result = HRESULT_FROM_WIN32(ERROR_BUSY);
        else *sequence = ++m_reserved;
        ReleaseSRWLockExclusive(&m_lock);
        return result;
    }

    HRESULT SequenceDomain::Settle(uint64_t sequence) noexcept
    {
        AcquireSRWLockExclusive(&m_lock);
        HRESULT result = S_OK;
        if (sequence <= m_settled || sequence > m_reserved || m_completed[sequence % Window]) result = E_INVALIDARG;
        else
        {
            m_completed[sequence % Window] = true;
            while (m_settled < m_reserved && m_completed[(m_settled + 1) % Window])
            {
                m_completed[(m_settled + 1) % Window] = false;
                ++m_settled;
            }
        }
        ReleaseSRWLockExclusive(&m_lock);
        return result;
    }

    void SequenceDomain::Snapshot(uint64_t* reserved, uint64_t* settled) noexcept
    {
        AcquireSRWLockShared(&m_lock);
        *reserved = m_reserved;
        *settled = m_settled;
        ReleaseSRWLockShared(&m_lock);
    }

    bool IsEnabled(Event event) noexcept
    {
        const uint64_t keyword = EventKeyword(event);
        if (keyword == 0 || (static_cast<uint64_t>(InterlockedCompareExchange64(&s_fastDefinitions, 0, 0)) &
            (uint64_t{1} << static_cast<uint16_t>(event))) == 0) return false;
        return DefinitionEnabled(event);
    }
    bool EnterInstrumentation() noexcept
    {
        if (s_insideInstrumentation) return false;
        s_observerBeforeInstrumentation = ObserverSuppression::Exchange(true);
        s_insideInstrumentation = true;
        return true;
    }
    void LeaveInstrumentation() noexcept
    {
        s_insideInstrumentation = false;
        ObserverSuppression::Exchange(s_observerBeforeInstrumentation);
    }
    HRESULT BeginEmission(Event event, Emission* emission) noexcept
    {
        if (!emission) return E_POINTER;
        InstrumentationScope scope;
        HRESULT result = EnsureEpoch();
        if (FAILED(result)) return result;
        // Publication is opportunistic, not another definition's enable gate.
        (void)PublishCapabilities();
        AcquireSRWLockShared(&s_stateLock);
        if (s_epochGeneration != InterlockedCompareExchange64(&s_enableGeneration, 0, 0))
        {
            ReleaseSRWLockShared(&s_stateLock);
            return HRESULT_FROM_WIN32(ERROR_RETRY);
        }
        if (!IsEnabled(event))
        {
            ReleaseSRWLockShared(&s_stateLock);
            return S_FALSE;
        }
        emission->Info = s_info;
        emission->Definition = event;
        emission->Sequence = &s_domains[static_cast<uint16_t>(event)];
        result = emission->Sequence->Reserve(&emission->SourceSequence);
        if (FAILED(result))
        {
            // Dropping an observation before reserving it cannot be represented by
            // a sequence gap. Stop visibly instead of silently degrading coverage.
            InterlockedExchange(&s_providerFault, HRESULT_CODE(result));
            ReleaseSRWLockShared(&s_stateLock);
        }
        else emission->OwnsInstrumentation = scope.Detach();
        return result;
    }
    void EndEmission(Emission* emission) noexcept
    {
        emission->Sequence->Settle(emission->SourceSequence);
        ReleaseSRWLockShared(&s_stateLock);
        if (emission->OwnsInstrumentation) LeaveInstrumentation();
    }
    HRESULT NewObjectIdentity(Domain domain, ObjectIdentity* identity) noexcept
    {
        if (!identity) return E_POINTER;
        *identity = {};
        if (static_cast<uint8_t>(domain) > 4) return E_INVALIDARG;
        InstrumentationScope scope;
        if (!IsEnabled(Event::ObjectLifetime) && !IsEnabled(Event::TreeEdge) && !IsEnabled(Event::VisualDelta) &&
            !IsEnabled(Event::Resource) && !IsEnabled(Event::CaptureChunk)) return S_FALSE;
        HRESULT result = EnsureEpoch();
        if (FAILED(result)) return result;
        AcquireSRWLockExclusive(&s_stateLock);
        if (s_objectId == UINT64_MAX)
        {
            InterlockedExchange(&s_providerFault, ERROR_ARITHMETIC_OVERFLOW);
            result = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }
        else *identity = {domain, s_info.ProducerEpoch, ++s_objectId};
        ReleaseSRWLockExclusive(&s_stateLock);
        return result;
    }

    namespace AllocatorObservation
    {
        AllocatorRuntime::~AllocatorRuntime() noexcept { ReleaseAllocatorOrigin(m_origin); }
        bool AllocatorRuntime::Enabled() noexcept { return IsEnabled(Event::Allocation); }
        bool AllocatorRuntime::GetContext(GUID& epoch, uint64_t& generation) noexcept
        {
            ObserverSuppression::Scope observer;
            InstrumentationScope instrumentation;
            ReleaseAllocatorOrigin(m_origin);
            m_origin = 0;
            LARGE_INTEGER begin{};
            QueryPerformanceCounter(&begin);
            m_observationStartQpc = begin.QuadPart;
            for (unsigned attempt = 0; attempt != 3; ++attempt)
            {
                const HRESULT result = EnsureEpoch();
#ifdef WINUI_PROFILER_STANDALONE
                if (s_allocatorContextRaceForTest)
                {
                    --s_allocatorContextRaceForTest;
                    ConfigureAllocatorForTest(false);
                    ConfigureAllocatorForTest(true);
                }
#endif
                if (FAILED(result))
                {
                    if (Enabled()) InterlockedCompareExchange(&s_providerFault, HRESULT_CODE(result), 0);
                    return false;
                }
                AcquireSRWLockExclusive(&s_stateLock);
                if (!m_origin || s_allocatorOrigins[m_origin - 1].Epoch != s_info.ProducerEpoch)
                {
                    ReleaseAllocatorOrigin(m_origin);
                    m_origin = AcquireAllocatorOriginLocked();
                }
                const LONG64 current = InterlockedCompareExchange64(&s_allocatorGeneration, 0, 0);
                const bool valid = m_origin && current > 0 && s_epochGeneration ==
                    InterlockedCompareExchange64(&s_enableGeneration, 0, 0);
                HRESULT mapping = S_OK;
                if (valid && (s_allocatorEpoch == GUID_NULL || s_allocatorMappedSelection != current ||
                    s_allocatorProducerEpoch != s_info.ProducerEpoch))
                {
                    GUID fresh{};
                    mapping = s_allocatorContextGeneration == UINT64_MAX ?
                        HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW) : CoCreateGuid(&fresh);
                    if (SUCCEEDED(mapping) && (fresh == GUID_NULL || fresh == s_info.ProducerEpoch ||
                        fresh == s_allocatorEpoch)) mapping = E_UNEXPECTED;
                    if (SUCCEEDED(mapping))
                    {
                        s_allocatorEpoch = fresh;
                        s_allocatorProducerEpoch = s_info.ProducerEpoch;
                        s_allocatorMappedSelection = current;
                        ++s_allocatorContextGeneration;
                    }
                }
                if (valid && SUCCEEDED(mapping))
                {
                    epoch = s_allocatorEpoch;
                    generation = s_allocatorContextGeneration;
                }
                ReleaseSRWLockExclusive(&s_stateLock);
                if (FAILED(mapping))
                {
                    InterlockedCompareExchange(&s_providerFault, HRESULT_CODE(mapping), 0);
                    return false;
                }
                if (valid) return Enabled();
                if (!Enabled()) return false;
            }
            // A skipped context proves a coverage gap, not a successful heap operation.
            AcquireSRWLockExclusive(&s_allocatorLossLock);
            QueueAllocatorLossLocked(m_origin, m_observationStartQpc, 1);
            ReleaseSRWLockExclusive(&s_allocatorLossLock);
            return false;
        }
        Registry<16384>& AllocatorRuntime::GetRegistry() noexcept
        {
            // First called only inside HeapAdapter's suppression scope.
            static Registry<16384> registry;
            return registry;
        }
        bool AllocatorRuntime::Emit(const Allocation& payload, uint64_t generation) noexcept
        {
            Emission emission;
            if (BeginEmission(Event::Allocation, &emission) != S_OK) return false;
            // BeginEmission holds s_stateLock shared: neither mapping nor producer
            // can change until EndEmission. Callbacks only invalidate the atomic token.
            const bool current = payload.AllocatorEpoch == s_allocatorEpoch &&
                generation == s_allocatorContextGeneration &&
                emission.Info.ProducerEpoch == s_allocatorProducerEpoch &&
                s_allocatorMappedSelection == InterlockedCompareExchange64(&s_allocatorGeneration, 0, 0);
            const HRESULT written = current ? Write(emission, payload) : HRESULT_FROM_WIN32(ERROR_RETRY);
            EndEmission(&emission);
            // A reserved but unwritten sequence is accounted by Delivery(false)
            // and ReportLoss, AFTER EndEmission releases the nonrecursive state lock.
            return written == S_OK;
        }
        void AllocatorRuntime::ReportLoss(uint64_t cumulative, uint64_t) noexcept
        {
            ObserverSuppression::Scope observer;
            if (cumulative == UINT64_MAX)
            {
                InterlockedCompareExchange(&s_providerFault, ERROR_ARITHMETIC_OVERFLOW, 0);
                return;
            }
            if (cumulative <= s_allocatorQueuedRegistryLoss &&
                InterlockedCompareExchange(&s_allocatorPendingOrigins, 0, 0) == 0) return;
            if (!m_origin && cumulative > s_allocatorQueuedRegistryLoss)
            {
                GUID epoch{};
                uint64_t generation = 0;
                if (!GetContext(epoch, generation)) return;
            }
            AcquireSRWLockExclusive(&s_allocatorLossLock);
            if (cumulative > s_allocatorQueuedRegistryLoss)
            {
                QueueAllocatorLossLocked(m_origin, m_observationStartQpc, cumulative - s_allocatorQueuedRegistryLoss);
                s_allocatorQueuedRegistryLoss = cumulative;
            }
            const DomainReference domains[] = {{12, EventVersion}};
            for (uint16_t i = 0; i != AllocatorOriginCapacity; ++i)
            {
                auto& pending = s_allocatorPendingLoss[i];
                if (!pending.CoverageGaps) continue;
                HRESULT result = EnsureEpoch();
                if (SUCCEEDED(result)) (void)PublishCapabilities();
                if (SUCCEEDED(result))
                {
                    AcquireSRWLockShared(&s_stateLock);
                    if (s_epochGeneration != InterlockedCompareExchange64(&s_enableGeneration, 0, 0))
                        result = HRESULT_FROM_WIN32(ERROR_RETRY);
                    else if (!IsEnabled(Event::Loss)) result = S_FALSE;
                    else
                    {
                        Emission emission;
                        auto& origin = s_allocatorOrigins[i];
                        emission.Info = s_info;
                        emission.Info.ProducerEpoch = origin.Epoch;
                        emission.Definition = Event::Loss;
                        if (origin.Retired)
                        {
                            if (origin.LastLossSequence == UINT64_MAX)
                                result = HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
                            else emission.SourceSequence = ++origin.LastLossSequence;
                        }
                        else
                        {
                            emission.Sequence = &s_domains[15];
                            result = emission.Sequence->Reserve(&emission.SourceSequence);
                        }
                        if (result == S_OK)
                        {
                            LARGE_INTEGER now{};
                            QueryPerformanceCounter(&now);
                            emission.TimestampQpc = now.QuadPart;
                            Loss loss;
                            loss.StartQpc = pending.StartQpc;
                            loss.EndQpc = now.QuadPart;
                            loss.Reason = "Partial allocator coverage: unmatched, stale, skipped or unwritten observations; event count unknown";
                            loss.HasDomains = true;
                            loss.Domains = {domains, 1};
                            result =
#ifdef WINUI_PROFILER_STANDALONE
                                s_allocatorFailLossWriteForTest ? E_ACCESSDENIED :
#endif
                                Write(emission, loss);
                            if (emission.Sequence) emission.Sequence->Settle(emission.SourceSequence);
                        }
                    }
                    ReleaseSRWLockShared(&s_stateLock);
                }
                if (result == S_OK)
                {
                    if (UINT64_MAX - s_allocatorReportedLoss < pending.CoverageGaps)
                        InterlockedCompareExchange(&s_providerFault, ERROR_ARITHMETIC_OVERFLOW, 0);
                    else s_allocatorReportedLoss += pending.CoverageGaps;
                    pending = {};
                    InterlockedDecrement(&s_allocatorPendingOrigins);
                    ReleaseAllocatorOrigin(i + 1);
                }
                else if (result != S_FALSE && result != HRESULT_FROM_WIN32(ERROR_CANCELLED) &&
                    result != HRESULT_FROM_WIN32(ERROR_RETRY))
                    InterlockedCompareExchange(&s_providerFault,
                        FAILED(result) ? HRESULT_CODE(result) : ERROR_WRITE_FAULT, 0);
            }
            ReleaseSRWLockExclusive(&s_allocatorLossLock);
        }
#ifdef WINUI_PROFILER_STANDALONE
        void ConfigureAllocatorForTest(bool enabled, bool lossSelected) noexcept
        {
            const LONG64 mask = enabled ? ((LONG64{1} << 12) | (lossSelected ? (LONG64{1} << 15) : 0)) : 0;
            InterlockedExchange64(&s_fastDefinitions, mask);
            InterlockedExchange64(&s_enabledDefinitions, mask);
            InterlockedExchange64(&s_cachedEligibilitySerial, s_capabilitiesSerial);
            InterlockedIncrement64(&s_allocatorGeneration);
            InterlockedIncrement64(&s_enableGeneration);
        }
        uint64_t AllocatorReportedLossForTest() noexcept { return s_allocatorReportedLoss; }
        void ArmAllocatorContextRaceForTest(unsigned attempts) noexcept { s_allocatorContextRaceForTest = attempts; }
        void FailAllocatorLossWriteForTest() noexcept { s_allocatorFailLossWriteForTest = true; }
        void RenewAllocatorSelectionForTest() noexcept { InterlockedIncrement64(&s_allocatorGeneration); }
        void RenewAllocatorProducerForTest() noexcept { InterlockedIncrement64(&s_enableGeneration); }
#endif
    }

    namespace Hooks
    {
        namespace
        {
            struct Key
            {
                Domain Kind;
                uintptr_t Address;
                bool operator<(const Key& other) const noexcept
                {
                    return Kind == other.Kind ? Address < other.Address : Kind < other.Kind;
                }
            };
            struct Entry
            {
                ObjectIdentity Identity;
                Optional<uint64_t> TypeId;
                Optional<Classification> TypeClassification;
                int64_t AnnouncedGeneration = -1;
                Microsoft::WRL::ComPtr<IWeakReference> Weak;
            };
            struct Root
            {
                GUID Id{};
                const void* Dispatcher = nullptr;
                uint64_t DispatcherId = 0;
                Optional<Attempt> Pending;
                bool CompositionPending = false;
            };
            struct Edge
            {
                const void* Parent = nullptr;
                const void* RootAddress = nullptr;
                uint8_t Relationship = 0;
                int32_t Order = 0;
                uint8_t Membership = 0;
            };
            SRWLOCK registryLock = SRWLOCK_INIT;
            enum RuntimeState : LONG { RuntimeClosed, RuntimeOpen, RuntimeTerminating };
            volatile LONG runtimeState = RuntimeClosed;
            LONG ReadRuntimeState() noexcept { return InterlockedCompareExchange(&runtimeState, 0, 0); }
            // Raw ownership has no CRT destructor. Only process termination may abandon it;
            // orderly shutdown destroys every container while PAL is still available.
            template<typename Container>
            class RegistryContainer
            {
                Container* value = nullptr;
                Container& Get()
                {
                    if (!value) value = new Container;
                    return *value;
                }
            public:
                auto begin() { return Get().begin(); }
                auto end() { return Get().end(); }
                auto find(const typename Container::key_type& key) { return Get().find(key); }
                auto size() { return value ? value->size() : 0; }
                void clear() { if (value) value->clear(); }
                void Reset() noexcept { delete std::exchange(value, nullptr); }
                void Abandon() noexcept { value = nullptr; }
                auto erase(typename Container::const_iterator position) { return Get().erase(position); }
                auto erase(const typename Container::key_type& key) { return Get().erase(key); }
                auto insert(const typename Container::value_type& item) { return Get().insert(item); }
                template<typename... Args>
                auto emplace(Args&&... args) { return Get().emplace(std::forward<Args>(args)...); }
                decltype(auto) operator[](const typename Container::key_type& key) { return Get()[key]; }
            };
            RegistryContainer<std::map<Key, Entry>> objects;
            static_assert(std::is_trivially_destructible_v<decltype(objects)>);
            // Epoch is the registry's enclosing key; Refresh retires all slots when it changes.
            using SlotKey = std::tuple<Domain, uint64_t, uint32_t>;
            struct SlotValue { ObjectIdentity Owner; uintptr_t Canonical; const void* Group = nullptr; };
            RegistryContainer<std::map<SlotKey, SlotValue>> visualSlots;
            RegistryContainer<std::map<uintptr_t, std::set<SlotKey>>> visualOwners;
            using VisualLinkKey = std::tuple<Domain, uint64_t, uint64_t>;
            RegistryContainer<std::map<VisualLinkKey, int64_t>> visualAnnouncements;
            RegistryContainer<std::map<std::pair<uint64_t, uint64_t>, TreeEdge>> visualEdgeContexts;
            RegistryContainer<std::set<uint64_t>> types;
            RegistryContainer<std::map<const void*, Root>> roots;
            RegistryContainer<std::map<const void*, uint64_t>> dispatchers;
            using EdgeKey = std::tuple<const void*, uint8_t, uint8_t>;
            EdgeKey MembershipKey(const void* child, uint8_t membership, uint8_t relationship) noexcept
            {
                return {child, membership, membership == 1 ? relationship : uint8_t{0}};
            }
            RegistryContainer<std::map<EdgeKey, Edge>> edges;
            struct LayoutOperation { const void* Element; uint8_t Kind; uint64_t Id; Cause ChangeCause; };
            RegistryContainer<std::map<DWORD, std::vector<LayoutOperation>>> layoutStacks;
            using GraphicsDeviceKey = std::pair<bool, uintptr_t>;
            struct GraphicsDeviceEntry { GUID Epoch{}; bool Announced = false; };
            struct GraphicsSurfaceEntry
            {
                Resource Value;
                GraphicsDeviceKey DeviceKey{};
                bool Announced = false;
                bool OwnsSurface = true;
            };
            RegistryContainer<std::map<GraphicsDeviceKey, GraphicsDeviceEntry>> graphicsDevices;
            RegistryContainer<std::map<const void*, GraphicsSurfaceEntry>> graphicsSurfaces;
            int64_t resourceGeneration = -1;
#ifdef WINUI_PROFILER_STANDALONE
            Event graphicsFenceDefinition = Event::Resource;
            HANDLE graphicsFenceReady = nullptr, graphicsFenceResume = nullptr;
#endif
            thread_local const void* currentRoot = nullptr;
            thread_local const void* currentDispatcher = nullptr;
            GUID epoch{};
            int64_t generation = -1;
            int64_t lifetimeGeneration = -1;
            int64_t frameGeneration = -1;
            int64_t slotGeneration = -1;
            int64_t correlationGeneration = -1;
            int64_t treeGeneration = -1;
            thread_local GUID layoutEpoch{};
            thread_local int64_t layoutFrameGeneration = -1;
            uint64_t nextOperation = 0;
            uint64_t submittedFrame = 0;
            constexpr size_t MaximumObjects = 65536;

            struct Lock
            {
                ObserverSuppression::Scope Observer;
                Lock() noexcept { AcquireSRWLockExclusive(&registryLock); }
                ~Lock() { ReleaseSRWLockExclusive(&registryLock); }
            };
            void RecordFailure(HRESULT result) noexcept
            {
                if (FAILED(result) && result != HRESULT_FROM_WIN32(ERROR_CANCELLED) &&
                    result != HRESULT_FROM_WIN32(ERROR_RETRY))
                    InterlockedCompareExchange(&s_providerFault, HRESULT_CODE(result), 0);
            }
            template<typename Payload>
            void Emit(Event definition, const Payload& payload, int64_t timestampQpc = 0) noexcept
            {
                if (!IsEnabled(definition)) return;
                Emission emission;
                HRESULT result = BeginEmission(definition, &emission);
                if (result == S_OK)
                {
                    emission.TimestampQpc = timestampQpc;
                    result = Write(emission, payload);
                    EndEmission(&emission);
                }
                RecordFailure(result);
            }
            void RecordVisualWeakFailure(HRESULT result) noexcept
            {
                if (result != E_NOINTERFACE)
                {
                    RecordFailure(result);
                    return;
                }
                if (!IsEnabled(Event::Loss)) return;
                LARGE_INTEGER now{};
                QueryPerformanceCounter(&now);
                const DomainReference domains[] = {{6, EventVersion}, {7, EventVersion}};
                Loss loss;
                loss.StartQpc = loss.EndQpc = now.QuadPart;
                loss.Reason = "Partial CompositionVisual coverage: weak references unsupported (HRESULT=0x80004002); visual identity not retained";
                loss.HasDomains = true;
                loss.Domains = {domains, ARRAYSIZE(domains)};
                Emit(Event::Loss, loss);
            }
            bool Refresh() noexcept
            {
                ProtocolInfo info;
                const HRESULT result = GetProtocolInfo(ProtocolVersion, sizeof(info), &info);
                if (FAILED(result)) { RecordFailure(result); return false; }
                const int64_t current = InterlockedCompareExchange64(&s_identityGeneration, 0, 0);
                if (epoch != info.ProducerEpoch)
                {
                    graphicsDevices.clear();
                    graphicsSurfaces.clear();
                    objects.clear();
                    visualSlots.clear();
                    visualOwners.clear();
                    visualAnnouncements.clear();
                    visualEdgeContexts.clear();
                    types.clear();
                    roots.clear();
                    dispatchers.clear();
                    edges.clear();
                    layoutStacks.clear();
                    nextOperation = 0;
                    submittedFrame = 0;
                    epoch = info.ProducerEpoch;
                    generation = current;
                }
                else if (generation != current)
                {
                    objects.clear();
                    visualSlots.clear();
                    visualOwners.clear();
                    visualAnnouncements.clear();
                    visualEdgeContexts.clear();
                    types.clear();
                    edges.clear();
                    generation = current;
                }
                const int64_t currentSlots = InterlockedCompareExchange64(&s_slotGeneration, 0, 0);
                const int64_t currentResources = InterlockedCompareExchange64(&s_resourceGeneration, 0, 0);
                if (resourceGeneration != currentResources)
                {
                    graphicsDevices.clear();
                    graphicsSurfaces.clear();
                    resourceGeneration = currentResources;
                }
                if (slotGeneration != currentSlots)
                {
                    visualSlots.clear();
                    visualOwners.clear();
                    visualAnnouncements.clear();
                    visualEdgeContexts.clear();
                    for (auto object = objects.begin(); object != objects.end();)
                    {
                        if (object->first.Kind == Domain::CompositionVisual) object = objects.erase(object);
                        else ++object;
                    }
                    slotGeneration = currentSlots;
                }
                const int64_t currentTrees = InterlockedCompareExchange64(&s_treeGeneration, 0, 0);
                if (treeGeneration != currentTrees)
                {
                    visualEdgeContexts.clear();
                    treeGeneration = currentTrees;
                }
                correlationGeneration = InterlockedCompareExchange64(&s_correlationGeneration, 0, 0);
                const int64_t currentLifetime = InterlockedCompareExchange64(&s_lifetimeGeneration, 0, 0);
                if (lifetimeGeneration != currentLifetime)
                {
                    types.clear();
                    lifetimeGeneration = currentLifetime;
                }
                const int64_t currentFrames = InterlockedCompareExchange64(&s_frameGeneration, 0, 0);
                if (frameGeneration != currentFrames)
                {
                    for (auto& root : roots) { root.second.Pending = {}; root.second.CompositionPending = false; }
                    frameGeneration = currentFrames;
                }
                return true;
            }
            uint64_t NextOperation() noexcept
            {
                if (nextOperation == UINT64_MAX)
                {
                    RecordFailure(HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
                    return 0;
                }
                return ++nextOperation;
            }
            Root* ObserveRoot(const void* address, const void* dispatcher)
            {
                if (!address) return nullptr;
                auto found = roots.find(address);
                if (found == roots.end())
                {
                    if (roots.size() == 1024)
                    {
                        RecordFailure(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                        return nullptr;
                    }
                    Root root;
                    RecordFailure(CoCreateGuid(&root.Id));
                    if (root.Id == GUID_NULL) return nullptr;
                    root.Dispatcher = dispatcher;
                    auto owner = dispatchers.find(dispatcher);
                    if (owner == dispatchers.end())
                    {
                        if (dispatchers.size() == 1024)
                        {
                            RecordFailure(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                            return nullptr;
                        }
                        owner = dispatchers.emplace(dispatcher, NextOperation()).first;
                    }
                    root.DispatcherId = owner->second;
                    found = roots.emplace(address, root).first;
                }
                return &found->second;
            }
            Root* PendingRoot(const void* address, const void* dispatcher, bool compositionWork = false)
            {
                Root* root = ObserveRoot(address, dispatcher);
                if (root && compositionWork) root->CompositionPending = true;
                if (root && FramesEnabled() && !root->Pending.Present)
                {
                    root->Pending = {true, {epoch, NextOperation(), root->Id, root->DispatcherId}};
                    Frame frame;
                    frame.Reference = root->Pending;
                    Emit(Event::Frame, frame);
                }
                return root;
            }
            Cause CurrentCause(const void* address = currentRoot) noexcept
            {
                const auto root = roots.find(address);
                if (root != roots.end() && root->second.Pending.Present)
                    return {2, root->second.Pending, nullptr};
                return {1, {}, nullptr};
            }
            void Announce(Entry& entry, const void* address, uint8_t operation) noexcept
            {
                if (!IsEnabled(Event::ObjectLifetime)) return;
                ObjectLifetime payload;
                payload.Object = entry.Identity;
                payload.Operation = operation;
                payload.TypeId = entry.TypeId;
                payload.Address = {true, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(address))};
                payload.TypeClassification = entry.TypeClassification;
                payload.DataQuality = operation == 2 ? Quality{0, 1, entry.Identity.IdentityDomain == Domain::CompositionVisual ?
                    "presence proven during observed ownership; actual birth/destruction and continuity outside this interval unknown" :
                    "presence observed; historical birth is unknown"} : Quality{};
                payload.ChangeCause = CurrentCause();
                Emit(Event::ObjectLifetime, payload);
                entry.AnnouncedGeneration = lifetimeGeneration;
            }
            Entry* Observe(Domain domain, const void* address, bool birth = false)
            {
                if (!address) return nullptr;
                const Key key{domain, reinterpret_cast<uintptr_t>(address)};
                auto found = objects.find(key);
                if (found != objects.end() && found->second.Weak)
                {
                    Microsoft::WRL::ComPtr<IInspectable> live;
                    const HRESULT result = found->second.Weak->Resolve(__uuidof(IInspectable), &live);
                    if (FAILED(result)) { RecordFailure(result); return nullptr; }
                    if (!live)
                    {
                        objects.erase(found);
                        found = objects.end();
                    }
                }
                if (birth && found != objects.end()) objects.erase(found);
                found = objects.find(key);
                if (found == objects.end())
                {
                    if (objects.size() == MaximumObjects)
                    {
                        RecordFailure(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                        return nullptr;
                    }
                    ObjectIdentity identity;
                    const HRESULT result = NewObjectIdentity(domain, &identity);
                    if (result != S_OK) { RecordFailure(result); return nullptr; }
                    Entry entry;
                    entry.Identity = identity;
                    if (domain == Domain::CompositionVisual)
                    {
                        Microsoft::WRL::ComPtr<IWeakReferenceSource> weakSource;
                        const HRESULT sourceResult = static_cast<IUnknown*>(const_cast<void*>(address))->QueryInterface(IID_PPV_ARGS(&weakSource));
                        if (FAILED(sourceResult)) { RecordVisualWeakFailure(sourceResult); return nullptr; }
                        const HRESULT weakResult = weakSource->GetWeakReference(&entry.Weak);
                        if (FAILED(weakResult)) { RecordVisualWeakFailure(weakResult); return nullptr; }
                    }
                    found = objects.emplace(key, std::move(entry)).first;
                }
                if (found->second.AnnouncedGeneration != lifetimeGeneration)
                {
                    found->second.TypeId = {};
                    found->second.TypeClassification = {};
                    Announce(found->second, address, birth ? 0 : 2);
                }
                return &found->second;
            }
            template<typename Action>
            void ObserveSafely(Action&& action) noexcept
            {
                if (ReadRuntimeState() != RuntimeOpen) return;
                if ((!IdentitiesEnabled() && !CompositionEnabled() && !FramesEnabled()) || !EnterInstrumentation()) return;
                struct InstrumentationGuard { ~InstrumentationGuard() { LeaveInstrumentation(); } } instrumentationGuard;
                Lock lock;
                if (ReadRuntimeState() != RuntimeOpen) return;
                try
                {
                    if (Refresh()) action();
                }
                catch (...) { RecordFailure(E_OUTOFMEMORY); }
            }
            struct GraphicsLastError
            {
                DWORD Value = GetLastError();
                ~GraphicsLastError() noexcept { SetLastError(Value); }
            };
            template<typename Payload>
            bool EmitGraphics(Event definition, const Payload& payload) noexcept
            {
                Emission emission;
                HRESULT result = BeginEmission(definition, &emission);
                if (result == S_OK)
                {
#ifdef WINUI_PROFILER_STANDALONE
                    if (graphicsFenceReady && graphicsFenceDefinition == definition)
                    {
                        const HANDLE ready = std::exchange(graphicsFenceReady, nullptr);
                        const HANDLE resume = std::exchange(graphicsFenceResume, nullptr);
                        SetEvent(ready);
                        if (WaitForSingleObject(resume, 10'000) != WAIT_OBJECT_0)
                            result = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                    }
                    if (result == S_OK)
#endif
                    result = emission.Info.ProducerEpoch == epoch &&
                        resourceGeneration == InterlockedCompareExchange64(&s_resourceGeneration, 0, 0) ?
                        Write(emission, payload) : HRESULT_FROM_WIN32(ERROR_RETRY);
                    EndEmission(&emission);
                }
                RecordFailure(result);
                return result == S_OK;
            }
            GraphicsDeviceEntry* GraphicsDeviceScope(const GraphicsDeviceKey& key, bool created = false)
            {
                auto found = graphicsDevices.find(key);
                if (found == graphicsDevices.end())
                {
                    if (graphicsDevices.size() == MaximumObjects)
                    {
                        RecordFailure(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                        return nullptr;
                    }
                    GraphicsDeviceEntry entry;
                    const HRESULT result = CoCreateGuid(&entry.Epoch);
                    if (FAILED(result)) { RecordFailure(result); return nullptr; }
                    found = graphicsDevices.emplace(key, entry).first;
                }
                auto& entry = found->second;
                if (key.first && !entry.Announced)
                {
                    Device payload;
                    payload.DeviceEpoch = entry.Epoch;
                    payload.Operation = created ? 0 : 3;
                    payload.Api = "D3D11 / CD3D11DeviceInstance";
                    payload.DataQuality = {0, 1, created ?
                        "Successful D3D11CreateDevice observed; external device ownership and GPU allocations are not covered" :
                        "Existing D3D11 device observed; creation history and external ownership are unavailable"};
                    entry.Announced = EmitGraphics(Event::Device, payload);
                }
                return &entry;
            }
        }
        void InitializeRuntime() noexcept
        {
            if (ReadRuntimeState() == RuntimeTerminating) return;
            Lock lock;
            if (ReadRuntimeState() != RuntimeTerminating) InterlockedExchange(&runtimeState, RuntimeOpen);
        }
        void ShutdownRuntime(bool processTerminating) noexcept
        {
            ObserverSuppression::Scope observer;
            if (processTerminating)
            {
                // ExitProcess has stopped other threads, possibly while they held registryLock.
                // Never lock, traverse, free, or call COM; the OS reclaims this dying process.
                InterlockedExchange(&runtimeState, RuntimeTerminating);
                objects.Abandon();
                visualSlots.Abandon();
                visualOwners.Abandon();
                visualAnnouncements.Abandon();
                visualEdgeContexts.Abandon();
                types.Abandon();
                roots.Abandon();
                dispatchers.Abandon();
                edges.Abandon();
                layoutStacks.Abandon();
                graphicsDevices.Abandon();
                graphicsSurfaces.Abandon();
                return;
            }
            if (ReadRuntimeState() == RuntimeTerminating) return;
            if (!EnterInstrumentation()) { RecordFailure(E_UNEXPECTED); return; }
            struct InstrumentationGuard { ~InstrumentationGuard() { LeaveInstrumentation(); } } instrumentationGuard;
            Lock lock;
            if (ReadRuntimeState() == RuntimeTerminating) return;
            InterlockedExchange(&runtimeState, RuntimeClosed);
            objects.Reset();
            visualSlots.Reset();
            visualOwners.Reset();
            visualAnnouncements.Reset();
            visualEdgeContexts.Reset();
            types.Reset();
            roots.Reset();
            dispatchers.Reset();
            edges.Reset();
            layoutStacks.Reset();
            graphicsDevices.Reset();
            graphicsSurfaces.Reset();
            // Keep epoch/ID counters: a later initialization cannot reuse qualified IDs.
        }
#ifdef WINUI_PROFILER_STANDALONE
        void ArmGraphicsEmissionFenceForTest(Event definition, HANDLE ready, HANDLE resume) noexcept
        {
            Lock lock;
            graphicsFenceDefinition = definition;
            graphicsFenceReady = ready;
            graphicsFenceResume = resume;
        }
        void GetGraphicsRegistryCountsForTest(size_t& devices, size_t& surfaces) noexcept
        {
            Lock lock;
            devices = graphicsDevices.size();
            surfaces = graphicsSurfaces.size();
        }
        void HoldEligibilityForTerminationTest(HANDLE ready, HANDLE resume) noexcept
        {
            AcquireSRWLockExclusive(&s_eligibilityLock);
            SetEvent(ready);
            WaitForSingleObject(resume, 10'000);
            ReleaseSRWLockExclusive(&s_eligibilityLock);
        }
        void HoldRegistryForTerminationTest(HANDLE ready) noexcept
        {
            Lock lock;
            SetEvent(ready);
            Sleep(INFINITE);
        }
        size_t LayoutStackCountForTest() noexcept
        {
            Lock lock;
            return layoutStacks.size();
        }
#endif
        bool IdentitiesEnabled() noexcept
        {
            return IsEnabled(Event::ObjectLifetime) || IsEnabled(Event::TreeEdge) ||
                IsEnabled(Event::VisualDelta) || IsEnabled(Event::Resource) || IsEnabled(Event::CaptureChunk);
        }
        bool TypesEnabled() noexcept { return IsEnabled(Event::TypeResolved); }
        bool FramesEnabled() noexcept { return IsEnabled(Event::Frame); }
        bool TreesEnabled() noexcept { return IsEnabled(Event::TreeEdge); }
        bool PropertiesEnabled() noexcept { return IsEnabled(Event::XamlPropertyChanged); }
        bool CompositionEnabled() noexcept { return IsEnabled(Event::Correlation); }
        void GraphicsDeviceCreated(const void* deviceOwner) noexcept
        {
            GraphicsLastError error;
            if (ReadRuntimeState() != RuntimeOpen || !deviceOwner || !IsEnabled(Event::Device)) return;
            ObserveSafely([&]
            {
                const GraphicsDeviceKey key{true, reinterpret_cast<uintptr_t>(deviceOwner)};
                auto previous = graphicsDevices.find(key);
                if (previous != graphicsDevices.end()) graphicsDevices.erase(previous);
                (void)GraphicsDeviceScope(key, true);
            });
        }
        void GraphicsDeviceReleased(const void* deviceOwner, bool lost) noexcept
        {
            GraphicsLastError error;
            if (ReadRuntimeState() != RuntimeOpen || !deviceOwner || !IsEnabled(Event::Device)) return;
            ObserveSafely([&]
            {
                const GraphicsDeviceKey key{true, reinterpret_cast<uintptr_t>(deviceOwner)};
                auto found = graphicsDevices.find(key);
                if (found == graphicsDevices.end()) return;
                if (found->second.Announced)
                {
                    Device payload;
                    payload.DeviceEpoch = found->second.Epoch;
                    payload.Api = "D3D11 / CD3D11DeviceInstance";
                    payload.DataQuality = {0, 1, "WinUI device ownership ended; final external COM/GPU destruction is unknown"};
                    if (lost)
                    {
                        payload.Operation = 1;
                        EmitGraphics(Event::Device, payload);
                    }
                    payload.Operation = 4;
                    EmitGraphics(Event::Device, payload);
                }
                graphicsDevices.erase(found);
            });
        }
        void GraphicsResourceChanged(const void* surface, const void* deviceOwner, GraphicsResourceKind kind,
            GraphicsSurfaceOperation operation, uint32_t width, uint32_t height, HRESULT result,
            const void* xamlOwner) noexcept
        {
            GraphicsLastError error;
            if (ReadRuntimeState() != RuntimeOpen || !surface || FAILED(result) || !IsEnabled(Event::Resource)) return;
            ObserveSafely([&]
            {
                if (kind != GraphicsResourceKind::Texture2D && kind != GraphicsResourceKind::CompositionSurface &&
                    kind != GraphicsResourceKind::CompositionBrush)
                {
                    RecordFailure(E_INVALIDARG);
                    return;
                }
                const bool texture = kind == GraphicsResourceKind::Texture2D;
                const bool brush = kind == GraphicsResourceKind::CompositionBrush;
                const Classification classification{static_cast<uint8_t>(kind), 3, brush ?
                    "Logical composition-brush ownership slot; not final COM object lifetime or a physical GPU allocation" : texture ?
                    "SystemMemoryBitsDriver logical staging-texture owner; not a physical GPU allocation" :
                    "DCompSurface logical wrapper; not a physical GPU allocation"};
                auto found = graphicsSurfaces.find(surface);
                const bool ending = operation == GraphicsSurfaceOperation::Destroy ||
                    operation == GraphicsSurfaceOperation::OwnershipRelease;
                if (found == graphicsSurfaces.end())
                {
                    if (ending) return;
                    if (graphicsSurfaces.size() == MaximumObjects)
                    {
                        RecordFailure(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
                        return;
                    }
                    GraphicsSurfaceEntry entry;
                    const HRESULT identity = NewObjectIdentity(Domain::Resource, &entry.Value.Object);
                    if (FAILED(identity)) { RecordFailure(identity); return; }
                    if (entry.Value.Object.Epoch != epoch) return;
                    entry.Value.ResourceClassification = classification;
                    found = graphicsSurfaces.emplace(surface, entry).first;
                }
                auto& entry = found->second;
                auto& payload = entry.Value;
                if (payload.ResourceClassification.Kind != classification.Kind)
                {
                    RecordFailure(E_INVALIDARG);
                    return;
                }
                if (entry.Announced && operation == GraphicsSurfaceOperation::BaselinePresent) return;
                if (!ending && xamlOwner && (!entry.Announced || operation == GraphicsSurfaceOperation::Rebind))
                {
                    auto* owner = Observe(Domain::XamlCore, xamlOwner);
                    if (!owner || owner->Identity.Epoch != payload.Object.Epoch) return;
                    payload.Owner = {true, owner->Identity};
                }
                if (!ending && (!entry.Announced || operation == GraphicsSurfaceOperation::Rebind))
                {
                    const GraphicsDeviceKey key{deviceOwner != nullptr,
                        reinterpret_cast<uintptr_t>(deviceOwner ? deviceOwner : surface)};
                    auto* device = GraphicsDeviceScope(key);
                    if (!device) return;
                    payload.DeviceEpoch = device->Epoch;
                    if (entry.DeviceKey != key && !entry.DeviceKey.first)
                        graphicsDevices.erase(entry.DeviceKey);
                    entry.DeviceKey = key;
                }
                if (!ending)
                {
                    payload.WidthPixels = {!brush, brush ? 0 : width};
                    payload.HeightPixels = {!brush, brush ? 0 : height};
                }
                payload.DataQuality = {0, 1, brush ?
                    "Logical composition-brush ownership slot only; hardware device, external COM lifetime and physical backing are unknown" :
                    texture ? (entry.DeviceKey.first ?
                    "Logical staging-texture owner only; physical backing, allocation bytes and external lifetime are unknown" :
                    "Logical staging-texture owner only; device association, physical backing and allocation bytes are unavailable") :
                    (entry.DeviceKey.first ?
                    "DCompSurface logical wrapper only; GPU backing, allocation bytes and external lifetime are unknown" :
                    "DCompSurface logical wrapper only; device association, GPU backing and allocation bytes are unavailable")};
                payload.ChangeCause = CurrentCause();
                if (!entry.Announced)
                {
                    payload.Operation = operation == GraphicsSurfaceOperation::Create ? 0 : 7;
                    entry.Announced = EmitGraphics(Event::Resource, payload);
                    if (!entry.Announced || operation == GraphicsSurfaceOperation::Create ||
                        operation == GraphicsSurfaceOperation::BaselinePresent) return;
                }
                if (operation == GraphicsSurfaceOperation::Create) operation = GraphicsSurfaceOperation::Rebind;
                if (operation != GraphicsSurfaceOperation::OwnershipRelease || entry.OwnsSurface)
                {
                    payload.Operation = static_cast<uint8_t>(operation);
                    EmitGraphics(Event::Resource, payload);
                }
                entry.OwnsSurface = !ending;
                if (operation == GraphicsSurfaceOperation::Destroy)
                {
                    if (!entry.DeviceKey.first) graphicsDevices.erase(entry.DeviceKey);
                    graphicsSurfaces.erase(found);
                }
            });
        }
        void GraphicsSurfaceChanged(const void* surface, const void* deviceOwner,
            GraphicsSurfaceOperation operation, uint32_t width, uint32_t height, HRESULT result) noexcept
        {
            GraphicsResourceChanged(surface, deviceOwner, GraphicsResourceKind::CompositionSurface,
                operation, width, height, result);
        }
        void Born(Domain domain, const void* object) noexcept
        {
            ObserveSafely([&] { Observe(domain, object, true); });
        }
        void Destroyed(Domain domain, const void* object) noexcept
        {
            ObserveSafely([&]
            {
                const Key key{domain, reinterpret_cast<uintptr_t>(object)};
                auto found = objects.find(key);
                if (found == objects.end())
                {
                    if (!Observe(domain, object)) return;
                    found = objects.find(key);
                }
                Announce(found->second, object, 1);
                objects.erase(found);
                for (auto edge = edges.begin(); edge != edges.end();)
                {
                    if (std::get<0>(edge->first) == object || edge->second.Parent == object) edge = edges.erase(edge);
                    else ++edge;
                }
            });
        }
        void ResolveCoreType(const void* core, uint64_t typeId, const wchar_t* name, bool uiElement) noexcept
        {
            if (!TypesEnabled()) return;
            ObserveSafely([&]
            {
                Entry* entry = Observe(Domain::XamlCore, core);
                if (!entry || (entry->TypeId.Present && entry->TypeId.Value == typeId)) return;
                char text[1025]{};
                if (!name || !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name, -1, text, ARRAYSIZE(text), nullptr, nullptr))
                {
                    RecordFailure(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION));
                    return;
                }
                entry->TypeId = {true, typeId};
                entry->TypeClassification = {true, {static_cast<uint8_t>(uiElement ? 2 : 1), 1, "core runtime type metadata"}};
                if (types.insert(typeId).second)
                    Emit(Event::TypeResolved, TypeResolved{typeId, text, entry->TypeClassification});
                // Refinement is a present-time observation, not another allocation.
                Announce(*entry, core, 2);
            });
        }
        void Associate(const void* core, const void* peer, bool linked) noexcept
        {
            if (!IsEnabled(Event::CorePeerAssociation) || !core || !peer) return;
            ObserveSafely([&]
            {
                const Entry* from = Observe(Domain::XamlCore, core);
                const Entry* to = Observe(Domain::XamlPeer, peer);
                if (from && to) Emit(Event::CorePeerAssociation, CorePeerAssociation{from->Identity, to->Identity, linked, CurrentCause()});
            });
        }
        void Work(const void* root, const void* dispatcher, bool compositionWork) noexcept
        {
            if (!FramesEnabled()) return;
            ObserveSafely([&] { PendingRoot(root, dispatcher, compositionWork); });
        }
        RootScope::RootScope(const void* root, const void* dispatcher, bool hasPendingWork) noexcept
        {
            if (!FramesEnabled() && !IdentitiesEnabled()) return;
            m_active = true;
            m_previous = currentRoot;
            m_previousDispatcher = currentDispatcher;
            if (hasPendingWork) Work(root, dispatcher, true);
            currentRoot = root;
            currentDispatcher = dispatcher;
        }
        RootScope::~RootScope() noexcept
        {
            if (m_active) { currentRoot = m_previous; currentDispatcher = m_previousDispatcher; }
        }
        BoundaryScope::BoundaryScope(uint8_t kind) noexcept : m_kind(kind)
        {
            if (!IsEnabled(Event::Boundary)) return;
            ObserveSafely([&]
            {
                m_id = NextOperation();
                m_cause = CurrentCause();
                Emit(Event::Boundary, Boundary{m_kind, 0, m_id, {}, m_cause});
            });
        }
        BoundaryScope::~BoundaryScope() noexcept
        {
            if (!m_id || !IsEnabled(Event::Boundary)) return;
            ObserveSafely([&] { Emit(Event::Boundary, Boundary{m_kind, 1, m_id, m_result, m_cause}); });
        }
        void ForgetRoot(const void* root) noexcept
        {
            if (!FramesEnabled() && !IdentitiesEnabled()) return;
            ObserveSafely([&]
            {
                const auto found = roots.find(root);
                if (found != roots.end() && found->second.Pending.Present)
                {
                    Frame frame;
                    frame.Reference = found->second.Pending;
                    frame.Outcome = 2;
                    frame.HResult = {true, E_ABORT};
                    Emit(Event::Frame, frame);
                }
                roots.erase(root);
            });
        }
        void Commit(const void* dispatcher, HRESULT result, bool primary) noexcept
        {
            if (!FramesEnabled()) return;
            LARGE_INTEGER timestamp;
            QueryPerformanceCounter(&timestamp);
            ObserveSafely([&]
            {
                if (!primary)
                {
                    Frame standalone;
                    standalone.Outcome = 3;
                    standalone.HResult = {true, result};
                    Emit(Event::Frame, standalone, timestamp.QuadPart);
                    return;
                }
                if (FAILED(result)) return;
                if (submittedFrame == UINT64_MAX)
                {
                    RecordFailure(HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
                    return;
                }
                const uint64_t id = ++submittedFrame;
                bool attributed = false;
                for (const auto& item : roots)
                {
                    if (item.second.Dispatcher != dispatcher || !item.second.Pending.Present || !item.second.CompositionPending) continue;
                    attributed = true;
                    Frame frame;
                    frame.Reference = item.second.Pending;
                    frame.Outcome = 1;
                    frame.SubmittedFrameId = {true, id};
                    frame.HResult = {true, result};
                    Emit(Event::Frame, frame, timestamp.QuadPart);
                }
                if (!attributed)
                {
                    Loss loss;
                    loss.StartQpc = timestamp.QuadPart;
                    loss.EndQpc = timestamp.QuadPart;
                    loss.Reason = "qualifying primary commit has no proven root attribution";
                    DomainReference domain{9, EventVersion};
                    loss.HasDomains = true;
                    loss.Domains = {&domain, 1};
                    Emit(Event::Loss, loss);
                }
            });
        }
        void CompleteBatch(const void* dispatcher, HRESULT result) noexcept
        {
            if (!FramesEnabled()) return;
            ObserveSafely([&]
            {
                for (auto& item : roots)
                {
                    if (item.second.Dispatcher != dispatcher || !item.second.Pending.Present) continue;
                    Frame frame;
                    frame.Reference = item.second.Pending;
                    frame.Outcome = FAILED(result) ? 2 : 4;
                    frame.HResult = {true, result};
                    Emit(Event::Frame, frame);
                    item.second.Pending = {};
                    item.second.CompositionPending = false;
                }
            });
        }
        void Membership(const void* rootAddress, const void* dispatcher, const void* parent, const void* child,
            uint8_t relationship, int32_t order, bool connected, uint8_t membership) noexcept
        {
            if (!TreesEnabled() || !child) return;
            ObserveSafely([&]
            {
                if (!rootAddress && !connected)
                {
                    const auto previous = edges.find(MembershipKey(child, membership, relationship));
                    if (previous != edges.end()) rootAddress = previous->second.RootAddress;
                }
                if (!rootAddress) return;
                Root* root = PendingRoot(rootAddress, dispatcher, true);
                Entry* childEntry = Observe(Domain::XamlCore, child);
                if (!root || !childEntry) return;
                TreeEdge edge;
                edge.Child = childEntry->Identity;
                if (parent)
                {
                    Entry* parentEntry = Observe(Domain::XamlCore, parent);
                    if (!parentEntry) return;
                    edge.Parent = {true, parentEntry->Identity};
                }
                edge.Relationship = relationship;
                edge.Order = order;
                edge.RootId = root->Id;
                edge.DispatcherId = root->DispatcherId;
                edge.Membership = membership;
                edge.ChangeCause = CurrentCause(rootAddress);
                edge.Connected = connected;
                const auto key = MembershipKey(child, membership, relationship);
                const auto previous = edges.find(key);
                if (!connected && previous != edges.end())
                {
                    edge.Relationship = previous->second.Relationship;
                    edge.Order = previous->second.Order;
                    if (previous->second.Parent)
                    {
                        Entry* oldParent = Observe(Domain::XamlCore, previous->second.Parent);
                        if (oldParent) edge.Parent = {true, oldParent->Identity};
                    }
                }
                if (connected && previous != edges.end() && previous->second.Parent == parent &&
                    previous->second.Order == order && previous->second.RootAddress == rootAddress &&
                    previous->second.Relationship == relationship) return;
                if (connected && previous != edges.end())
                {
                    TreeEdge old = edge;
                    if (const auto oldRoot = roots.find(previous->second.RootAddress); oldRoot != roots.end())
                    {
                        old.RootId = oldRoot->second.Id;
                        old.DispatcherId = oldRoot->second.DispatcherId;
                    }
                    if (previous->second.Parent)
                    {
                        Entry* oldParent = Observe(Domain::XamlCore, previous->second.Parent);
                        if (oldParent) old.Parent = {true, oldParent->Identity};
                    }
                    else old.Parent = {};
                    old.Relationship = previous->second.Relationship;
                    old.Order = previous->second.Order;
                    old.Connected = false;
                    Emit(Event::TreeEdge, old);
                }
                Emit(Event::TreeEdge, edge);
                if (connected) edges[key] = {parent, rootAddress, relationship, order, membership};
                else edges.erase(key);
            });
        }
        void Property(const void* root, const void* dispatcher, const void* object, uint64_t propertyId,
            const wchar_t* declaringType, const wchar_t* name, const Value& previous, const Value& current) noexcept
        {
            if (!PropertiesEnabled()) return;
            ObserveSafely([&]
            {
                if (root) PendingRoot(root, dispatcher);
                Entry* entry = Observe(Domain::XamlCore, object);
                if (!entry) return;
                char typeText[1025]{}, propertyText[1025]{};
                if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, declaringType, -1, typeText, ARRAYSIZE(typeText), nullptr, nullptr) ||
                    !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name, -1, propertyText, ARRAYSIZE(propertyText), nullptr, nullptr))
                {
                    RecordFailure(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION));
                    return;
                }
                XamlPropertyChanged payload;
                payload.Object = entry->Identity;
                payload.PropertyId = propertyId;
                payload.DeclaringType = typeText;
                payload.Name = propertyText;
                payload.Previous = previous;
                payload.Current = current;
                payload.ChangeCause = CurrentCause(root);
                Emit(Event::XamlPropertyChanged, payload);
            });
        }
        void Link(const void* from, Domain fromDomain, const void* to, Domain toDomain, uint8_t kind, bool linked) noexcept
        {
            if (!CompositionEnabled() || !from || !to) return;
            ObserveSafely([&]
            {
                Entry* source = Observe(fromDomain, from);
                Entry* target = Observe(toDomain, to);
                if (source && target) Emit(Event::Correlation, Correlation{source->Identity, target->Identity, kind, linked, CurrentCause()});
            });
        }
        namespace
        {
            bool SlotsEnabled() noexcept
            {
                return IsEnabled(Event::ObjectLifetime) || IsEnabled(Event::VisualDelta) ||
                    IsEnabled(Event::Correlation) || IsEnabled(Event::TreeEdge) || IsEnabled(Event::CaptureChunk);
            }
            void SlotCorrelation(const SlotValue& slot, bool linked)
            {
                const auto target = objects.find({Domain::CompositionVisual, slot.Canonical});
                if (target == objects.end()) return;
                const VisualLinkKey key{slot.Owner.IdentityDomain, slot.Owner.Id, target->second.Identity.Id};
                if (CompositionEnabled())
                {
                    const auto announced = visualAnnouncements.find(key);
                    const auto kind = static_cast<uint8_t>(slot.Owner.IdentityDomain == Domain::CompNode ? 2 : 3);
                    if (announced == visualAnnouncements.end() || announced->second != correlationGeneration)
                    {
                        Emit(Event::Correlation, Correlation{slot.Owner, target->second.Identity, kind, true, CurrentCause()});
                        visualAnnouncements[key] = correlationGeneration;
                    }
                    if (!linked)
                        Emit(Event::Correlation, Correlation{slot.Owner, target->second.Identity, kind, false, CurrentCause()});
                }
                if (!linked) visualAnnouncements.erase(key);
            }
            bool SameOwner(const SlotKey& left, const SlotKey& right) noexcept
            {
                return std::get<0>(left) == std::get<0>(right) && std::get<1>(left) == std::get<1>(right);
            }
            void RetireSlot(std::map<SlotKey, SlotValue>::iterator slot)
            {
                auto visual = visualOwners.find(slot->second.Canonical);
                if (visual == visualOwners.end()) { RecordFailure(E_UNEXPECTED); return; }
                visual->second.erase(slot->first);
                bool otherSlot = false;
                for (const auto& token : visual->second)
                    if (SameOwner(token, slot->first)) { otherSlot = true; break; }
                if (!otherSlot) SlotCorrelation(slot->second, false);
                if (visual->second.empty())
                {
                    const auto identity = objects.find({Domain::CompositionVisual, slot->second.Canonical});
                    if (identity != objects.end())
                    {
                        const auto id = identity->second.Identity.Id;
                        for (auto edge = visualEdgeContexts.begin(); edge != visualEdgeContexts.end();)
                        {
                            if (edge->first.first == id || edge->first.second == id) edge = visualEdgeContexts.erase(edge);
                            else ++edge;
                        }
                    }
                    objects.erase({Domain::CompositionVisual, slot->second.Canonical});
                    visualOwners.erase(visual);
                    LARGE_INTEGER now{};
                    QueryPerformanceCounter(&now);
                    const DomainReference domains[] = {{2, EventVersion}, {5, EventVersion}, {6, EventVersion}, {7, EventVersion}};
                    Loss loss;
                    loss.StartQpc = loss.EndQpc = now.QuadPart;
                    loss.Reason = "Partial CompositionVisual coverage: last observed owner released; actual destruction and subsequent continuity unknown";
                    loss.HasDomains = true;
                    loss.Domains = {domains, ARRAYSIZE(domains)};
                    Emit(Event::Loss, loss);
                }
                visualSlots.erase(slot);
            }
        }
        void ReleaseVisualSlotGroup(const void* group) noexcept
        {
            if (!SlotsEnabled() || !group) return;
            ObserveSafely([&]
            {
                for (auto slot = visualSlots.begin(); slot != visualSlots.end();)
                {
                    auto current = slot++;
                    if (current->second.Group == group) RetireSlot(current);
                }
            });
        }
        void VisualSlot(const void* owner, Domain domain, uint32_t slot, IUnknown* visual, const void* group) noexcept
        {
            if (!SlotsEnabled() || !owner) return;
            ObserveSafely([&]
            {
                if (domain != Domain::CompNode && domain != Domain::XamlCore) { RecordFailure(E_INVALIDARG); return; }
                Entry* source = nullptr;
                const auto existingOwner = objects.find({domain, reinterpret_cast<uintptr_t>(owner)});
                if (!visual)
                {
                    if (existingOwner == objects.end()) return;
                    const auto existing = visualSlots.find({domain, existingOwner->second.Identity.Id, slot});
                    if (existing != visualSlots.end() && (!group || existing->second.Group == group)) RetireSlot(existing);
                    return;
                }
                source = Observe(domain, owner);
                if (!source) return;
                Microsoft::WRL::ComPtr<IUnknown> canonical;
                const HRESULT result = visual->QueryInterface(IID_PPV_ARGS(&canonical));
                if (FAILED(result)) { RecordFailure(result); return; }
                if (!canonical) { RecordFailure(E_UNEXPECTED); return; }
                const uintptr_t address = reinterpret_cast<uintptr_t>(canonical.Get());
                const SlotKey token{domain, source->Identity.Id, slot};
                auto previous = visualSlots.find(token);
                if (previous != visualSlots.end() && previous->second.Canonical == address)
                {
                    previous->second.Group = group;
                    auto found = objects.find({Domain::CompositionVisual, address});
                    if (found != objects.end() && found->second.AnnouncedGeneration != lifetimeGeneration)
                        Announce(found->second, canonical.Get(), 2);
                    SlotCorrelation(previous->second, true);
                    return;
                }
                if (visualSlots.size() >= MaximumObjects || objects.size() >= MaximumObjects)
                { RecordFailure(E_OUTOFMEMORY); return; }
                auto target = objects.find({Domain::CompositionVisual, address});
                if (target == objects.end())
                {
                    Entry entry;
                    const HRESULT allocated = NewObjectIdentity(Domain::CompositionVisual, &entry.Identity);
                    if (allocated != S_OK) { RecordFailure(allocated); return; }
                    target = objects.emplace(Key{Domain::CompositionVisual, address}, std::move(entry)).first;
                }
                if (target->second.AnnouncedGeneration != lifetimeGeneration) Announce(target->second, canonical.Get(), 2);
                auto& owners = visualOwners[address];
                owners.insert(token);
                const SlotValue replacement{source->Identity, address, group};
                // Incoming native reference is live before the old slot is released.
                if (previous != visualSlots.end()) RetireSlot(previous);
                visualSlots.emplace(token, replacement);
                SlotCorrelation(replacement, true);
            });
        }
        void ReleaseVisualOwner(const void* owner, Domain domain) noexcept
        {
            if (!SlotsEnabled() || !owner) return;
            ObserveSafely([&]
            {
                const auto source = objects.find({domain, reinterpret_cast<uintptr_t>(owner)});
                if (source == objects.end()) return;
                for (auto slot = visualSlots.begin(); slot != visualSlots.end();)
                {
                    auto current = slot++;
                    if (std::get<0>(current->first) == domain &&
                        std::get<1>(current->first) == source->second.Identity.Id)
                        RetireSlot(current);
                }
            });
        }
        void LinkVisual(const void* node, IUnknown* visual) noexcept
        {
            if (!CompositionEnabled() || !visual) return;
            ObserverSuppression::Scope observer;
            IUnknown* canonical = nullptr;
            const HRESULT result = visual->QueryInterface(IID_PPV_ARGS(&canonical));
            if (SUCCEEDED(result))
            {
                Link(node, Domain::CompNode, canonical, Domain::CompositionVisual, 2, true);
                canonical->Release();
            }
            else RecordFailure(result);
        }
        void VisualProperty(IUnknown* visual, uint8_t property, double value) noexcept
        {
            if (!IsEnabled(Event::VisualDelta) || !visual) return;
            ObserveSafely([&]
            {
                PendingRoot(currentRoot, currentDispatcher, true);
                IUnknown* canonical = nullptr;
                const HRESULT result = visual->QueryInterface(IID_PPV_ARGS(&canonical));
                if (FAILED(result)) { RecordFailure(result); return; }
                Entry* entry = Observe(Domain::CompositionVisual, canonical);
                canonical->Release();
                if (!entry) return;
                Value typed;
                typed.Kind = 1;
                typed.Unit = property <= 3 ? 4 : 6;
                typed.Number = value;
                typed.DataQuality = {};
                Emit(Event::VisualDelta, VisualDelta{entry->Identity, property, typed, CurrentCause()});
            });
        }
        void VisualMembership(IUnknown* parent, IUnknown* child, int32_t order, bool connected) noexcept
        {
            if (!TreesEnabled() || !parent || !child) return;
            ObserveSafely([&]
            {
                IUnknown* parentIdentity = nullptr;
                IUnknown* childIdentity = nullptr;
                HRESULT result = parent->QueryInterface(IID_PPV_ARGS(&parentIdentity));
                if (SUCCEEDED(result)) result = child->QueryInterface(IID_PPV_ARGS(&childIdentity));
                if (FAILED(result))
                {
                    if (parentIdentity) parentIdentity->Release();
                    RecordFailure(result);
                    return;
                }
                Entry* source = Observe(Domain::CompositionVisual, parentIdentity);
                Entry* target = Observe(Domain::CompositionVisual, childIdentity);
                parentIdentity->Release();
                childIdentity->Release();
                if (!source || !target) return;
                const auto key = std::make_pair(source->Identity.Id, target->Identity.Id);
                const auto previous = visualEdgeContexts.find(key);
                Root* root = connected ? ObserveRoot(currentRoot, currentDispatcher) : nullptr;
                if ((connected && !root) || (!connected && previous == visualEdgeContexts.end()))
                {
                    const DomainReference domains[] = {{5, EventVersion}};
                    LARGE_INTEGER now{};
                    QueryPerformanceCounter(&now);
                    Loss loss;
                    loss.StartQpc = loss.EndQpc = now.QuadPart;
                    loss.Reason = "Partial VisualChild coverage: no proven qualified edge root/dispatcher context";
                    loss.HasDomains = true;
                    loss.Domains = {domains, ARRAYSIZE(domains)};
                    Emit(Event::Loss, loss);
                    return;
                }
                if (connected && previous == visualEdgeContexts.end() && visualEdgeContexts.size() >= MaximumObjects)
                { RecordFailure(E_OUTOFMEMORY); return; }
                TreeEdge edge;
                edge.Parent = {true, source->Identity};
                edge.Child = target->Identity;
                edge.Relationship = 8;
                edge.Order = order;
                edge.RootId = connected ? root->Id : previous->second.RootId;
                edge.DispatcherId = connected ? root->DispatcherId : previous->second.DispatcherId;
                edge.Membership = 3;
                edge.Connected = connected;
                edge.ChangeCause = CurrentCause();
                Emit(Event::TreeEdge, edge);
                if (connected) visualEdgeContexts[key] = edge;
                else visualEdgeContexts.erase(key);
            });
        }
        void Layout(const void* root, const void* dispatcher, const void* element, uint8_t kind, bool begin) noexcept
        {
            if (!IsEnabled(Event::Boundary)) return;
            ObserveSafely([&]
            {
                const DWORD thread = GetCurrentThreadId();
                auto stack = layoutStacks.find(thread);
                if (!begin && stack == layoutStacks.end()) return;
                if (begin && layoutStacks.size() >= 1024 && stack == layoutStacks.end())
                {
                    RecordFailure(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
                    return;
                }
                auto& layoutOperations = layoutStacks[thread];
                if (layoutEpoch != epoch || layoutFrameGeneration != frameGeneration)
                {
                    layoutOperations.clear();
                    layoutEpoch = epoch;
                    layoutFrameGeneration = frameGeneration;
                }
                if (!begin && layoutOperations.empty()) { layoutStacks.erase(thread); return; }
                if (begin)
                {
                    PendingRoot(root, dispatcher);
                    const uint64_t operation = NextOperation();
                    const Cause cause = CurrentCause(root);
                    layoutOperations.push_back({element, kind, operation, cause});
                    Emit(Event::Boundary, Boundary{kind, 0, operation, {}, cause});
                }
                else if (!layoutOperations.empty())
                {
                    const auto operation = layoutOperations.back();
                    if (operation.Element != element || operation.Kind != kind)
                    {
                        RecordFailure(E_UNEXPECTED);
                        return;
                    }
                    layoutOperations.pop_back();
                    Emit(Event::Boundary, Boundary{kind, 1, operation.Id, {}, operation.ChangeCause});
                    if (layoutOperations.empty()) layoutStacks.erase(GetCurrentThreadId());
                }
            });
        }
    }
    namespace
    {
        bool String(const char* value, bool required = false) noexcept
        {
            if (!value) return !required;
            const size_t length = strnlen_s(value, 1025);
            if (length > 1024 || (required && length == 0)) return false;
            if (length != 0 && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, static_cast<int>(length), nullptr, 0) == 0) return false;
            if (!required) return true;
            // Match shared Require.Text Unicode whitespace without allocating.
            for (size_t i = 0; i < length;)
            {
                uint32_t codePoint = static_cast<uint8_t>(value[i++]);
                if (codePoint >= 0x80)
                {
                    const uint32_t tails = codePoint < 0xe0 ? 1 : codePoint < 0xf0 ? 2 : 3;
                    codePoint &= (uint32_t{1} << (6 - tails)) - 1;
                    for (uint32_t tail = 0; tail < tails; ++tail)
                        codePoint = (codePoint << 6) | (static_cast<uint8_t>(value[i++]) & 0x3f);
                }
                const bool whitespace = (codePoint >= 9 && codePoint <= 13) || codePoint == 0x20 ||
                    codePoint == 0x85 || codePoint == 0xa0 || codePoint == 0x1680 ||
                    (codePoint >= 0x2000 && codePoint <= 0x200a) || codePoint == 0x2028 ||
                    codePoint == 0x2029 || codePoint == 0x202f || codePoint == 0x205f || codePoint == 0x3000;
                if (!whitespace) return true;
            }
            return false;
        }
        bool Valid(const Quality& value) noexcept
        {
            return value.Kind <= 2 && value.Coverage <= 2 && String(value.Reason, value.Kind != 0 || value.Coverage != 0);
        }
        bool Valid(const Classification& value, bool resource = false) noexcept
        {
            return value.Kind <= (resource ? 8 : 3) && value.Evidence <= (resource ? 3 : 2) &&
                ((value.Kind == 0) == (value.Evidence == 0)) && String(value.Source, true);
        }
        bool Valid(const ObjectIdentity& value) noexcept
        {
            return static_cast<uint8_t>(value.IdentityDomain) <= 4 && value.Epoch != GUID_NULL && value.Id != 0;
        }
        bool Valid(const Attempt& value) noexcept
        {
            return value.Epoch != GUID_NULL && value.Id != 0 && value.RootId != GUID_NULL && value.DispatcherId != 0;
        }
        bool Valid(const Cause& value) noexcept
        {
            return value.Kind <= 2 && (value.Reference.Present == (value.Kind == 2)) &&
                (!value.Reference.Present || Valid(value.Reference.Value)) && String(value.Reason, value.Kind == 0);
        }
        bool Valid(const Value& value) noexcept
        {
            if (value.Kind > 4 || value.Unit > 7 || !Valid(value.DataQuality) || !String(value.Text)) return false;
            if (value.Kind != 4 && value.DataQuality.Kind == 2) return false;
            switch (value.Kind)
            {
            case 0: return value.Unit != 5 && value.Unit != 7;
            case 1: return value.Unit != 5 && value.Unit != 7 && std::isfinite(value.Number);
            case 2: return value.Unit == 5;
            case 3: return value.Unit == 7 && value.Text != nullptr;
            default: return value.DataQuality.Kind == 2;
            }
        }
        bool InEpoch(const ObjectIdentity& object, const Emission& emission) noexcept { return object.Epoch == emission.Info.ProducerEpoch; }
        bool InEpoch(const Cause& cause, const Emission& emission) noexcept
        {
            return !cause.Reference.Present || cause.Reference.Value.Epoch == emission.Info.ProducerEpoch;
        }
        bool ValidClassification(Domain domain, const Optional<Classification>& classification) noexcept
        {
            return !classification.Present || (Valid(classification.Value) &&
                (domain != Domain::XamlCore || classification.Value.Kind != 3) &&
                (domain == Domain::XamlCore || domain == Domain::XamlPeer ||
                    (classification.Value.Kind != 1 && classification.Value.Kind != 2)));
        }
        bool Valid(const DomainReference& domain) noexcept
        {
            return domain.EventId >= 1 && domain.EventId <= 16 && domain.Version == EventVersion;
        }
        template<typename T, typename Predicate>
        bool ValidArray(const Array<T>& values, uint32_t maximum, Predicate valid) noexcept
        {
            if (values.Count > maximum || (values.Count && !values.Items)) return false;
            for (uint32_t i = 0; i < values.Count; ++i) if (!valid(values.Items[i])) return false;
            return true;
        }
        bool Valid(const SnapshotObject& value) noexcept
        {
            return Valid(value.Key) && String(value.TypeName) && (!value.Address.Present || value.Address.Value != 0) &&
                Valid(value.Size) && value.Size.Unit == 1 && (value.Size.Kind <= 1 || value.Size.Kind == 4) &&
                (value.Size.Kind != 0 || value.Size.Integer >= 0) && (value.Size.Kind != 1 || value.Size.Number >= 0) &&
                ValidClassification(value.Key.IdentityDomain, value.TypeClassification);
        }
        bool Valid(const OwnershipEdge& value) noexcept
        {
            return Valid(value.From) && Valid(value.To) && String(value.Relationship, true) &&
                value.Evidence <= 1 && Valid(value.DataQuality);
        }
        HRESULT Result(bool valid) noexcept { return valid ? S_OK : E_INVALIDARG; }
    }

    HRESULT Validate(const Frame& p) noexcept
    {
        return Result(p.Outcome <= 4 && (p.Outcome == 3 || p.Reference.Present) &&
            (!p.Reference.Present || Valid(p.Reference.Value)) &&
            (p.SubmittedFrameId.Present == (p.Outcome == 1)) &&
            (!p.SubmittedFrameId.Present || p.SubmittedFrameId.Value != 0) &&
            (p.Outcome != 0 || !p.HResult.Present) &&
            (p.Outcome != 4 || (p.HResult.Present && SUCCEEDED(p.HResult.Value))) &&
            (p.Outcome != 1 || (p.HResult.Present && SUCCEEDED(p.HResult.Value))) &&
            (p.Outcome != 2 || (p.HResult.Present && FAILED(p.HResult.Value))));
    }
    HRESULT Write(const Emission& emission, const Frame& payload) noexcept
    {
        if (payload.Reference.Present && payload.Reference.Value.Epoch != emission.Info.ProducerEpoch) return E_INVALIDARG;
        return WriteNative(emission, payload, Event::Frame);
    }
    HRESULT Validate(const ObjectLifetime& p) noexcept
    {
        return Result(Valid(p.Object) && p.Operation <= 2 && (!p.TypeId.Present || p.TypeId.Value != 0) &&
            (!p.Address.Present || p.Address.Value != 0) && Valid(p.DataQuality) &&
            ValidClassification(p.Object.IdentityDomain, p.TypeClassification) && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const ObjectLifetime& p) noexcept
    {
        if (!InEpoch(p.Object, emission) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::ObjectLifetime);
    }
    HRESULT Validate(const TypeResolved& p) noexcept
    {
        return Result(p.TypeId != 0 && String(p.Name, true) && (!p.TypeClassification.Present || Valid(p.TypeClassification.Value)));
    }
    HRESULT Write(const Emission& emission, const TypeResolved& p) noexcept
    {
        return WriteNative(emission, p, Event::TypeResolved);
    }
    HRESULT Validate(const CorePeerAssociation& p) noexcept
    {
        return Result(Valid(p.Core) && Valid(p.Peer) && p.Core.IdentityDomain == Domain::XamlCore &&
            p.Peer.IdentityDomain == Domain::XamlPeer && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const CorePeerAssociation& p) noexcept
    {
        if (!InEpoch(p.Core, emission) || !InEpoch(p.Peer, emission) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::CorePeerAssociation);
    }
    HRESULT Validate(const TreeEdge& p) noexcept
    {
        const bool root = p.Relationship == 0 || p.Relationship == 5 || p.Relationship == 7;
        const Domain domain = p.Relationship < 5 ? Domain::XamlCore : p.Relationship < 7 ? Domain::CompNode : Domain::CompositionVisual;
        return Result(Valid(p.Child) && p.Relationship <= 8 && p.Child.IdentityDomain == domain && p.Order >= 0 &&
            p.RootId != GUID_NULL && p.DispatcherId != 0 && p.Membership <= 3 &&
            (domain == Domain::XamlCore ? p.Membership != 3 : p.Membership == 0 || p.Membership == 3) &&
            p.Parent.Present != root && (!p.Parent.Present || (Valid(p.Parent.Value) &&
                p.Parent.Value.IdentityDomain == domain && p.Parent.Value.Id != p.Child.Id)) && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const TreeEdge& p) noexcept
    {
        if (!InEpoch(p.Child, emission) || (p.Parent.Present && !InEpoch(p.Parent.Value, emission)) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::TreeEdge);
    }
    HRESULT Validate(const VisualDelta& p) noexcept
    {
        return Result(Valid(p.Visual) && p.Visual.IdentityDomain == Domain::CompositionVisual && p.Property <= 9 &&
            Valid(p.PropertyValue) && p.PropertyValue.Unit == (p.Property == 5 ? 5 : p.Property < 4 ? 4 : 6) &&
            (p.PropertyValue.Kind == 4 || (p.Property == 5 ? p.PropertyValue.Kind == 2 : p.PropertyValue.Kind <= 1)) &&
            ((p.Property != 2 && p.Property != 3) || p.PropertyValue.Kind == 4 || (p.PropertyValue.Kind == 0 ?
                p.PropertyValue.Integer >= 0 : p.PropertyValue.Number >= 0)) &&
            (p.Property != 4 || p.PropertyValue.Kind == 4 || (p.PropertyValue.Kind == 0 ?
                p.PropertyValue.Integer >= 0 && p.PropertyValue.Integer <= 1 : p.PropertyValue.Number >= 0 && p.PropertyValue.Number <= 1)) &&
            Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const VisualDelta& p) noexcept
    {
        if (!InEpoch(p.Visual, emission) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::VisualDelta);
    }
    HRESULT Validate(const Correlation& p) noexcept
    {
        constexpr Domain from[] = {Domain::XamlCore, Domain::XamlCore, Domain::CompNode, Domain::XamlCore, Domain::CompositionVisual, Domain::Resource};
        constexpr Domain to[] = {Domain::XamlPeer, Domain::CompNode, Domain::CompositionVisual, Domain::CompositionVisual, Domain::Resource, Domain::Resource};
        return Result(p.Kind <= 5 && Valid(p.From) && Valid(p.To) && p.From.IdentityDomain == from[p.Kind] &&
            p.To.IdentityDomain == to[p.Kind] && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const Correlation& p) noexcept
    {
        if (!InEpoch(p.From, emission) || !InEpoch(p.To, emission) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::Correlation);
    }
    HRESULT Validate(const Boundary& p) noexcept
    {
        return Result(p.Kind <= 4 && p.Phase <= 1 && p.OperationId != 0 && (p.Phase != 0 || !p.HResult.Present) && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const Boundary& p) noexcept
    {
        if (!InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::Boundary);
    }
    HRESULT Validate(const Device& p) noexcept
    {
        return Result(p.DeviceEpoch != GUID_NULL && p.Operation <= 4 && String(p.Api, true) && Valid(p.DataQuality) &&
            ((p.Operation != 3 && p.Operation != 4) || p.DataQuality.Coverage != 0));
    }
    HRESULT Write(const Emission& emission, const Device& p) noexcept
    {
        return WriteNative(emission, p, Event::Device);
    }
    HRESULT Validate(const Resource& p) noexcept
    {
        return Result(Valid(p.Object) && p.Object.IdentityDomain == Domain::Resource && p.Operation <= 7 &&
            (p.Operation != 7 || p.DataQuality.Coverage != 0) &&
            p.DeviceEpoch != GUID_NULL && (!p.BackingAllocationId.Present || p.BackingAllocationId.Value != GUID_NULL) &&
            p.SizeProvenance <= 2 && (p.AllocationBytes.Present == (p.SizeProvenance != 2)) &&
            (p.SizeProvenance != 0 || (p.BackingAllocationId.Present && p.DataQuality.Kind == 0)) &&
            (p.SizeProvenance != 1 || p.DataQuality.Kind == 1) && (!p.Owner.Present || Valid(p.Owner.Value)) &&
            Valid(p.DataQuality) && Valid(p.ResourceClassification, true) && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const Resource& p) noexcept
    {
        if (!InEpoch(p.Object, emission) || (p.Owner.Present && !InEpoch(p.Owner.Value, emission)) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::Resource);
    }
    HRESULT Validate(const Allocation& p) noexcept
    {
        return Result(p.AllocationId != 0 && p.Operation <= 2 && p.AllocatorEpoch != GUID_NULL &&
            (p.PreviousAllocationId.Present == (p.Operation == 2)) && (!p.PreviousAllocationId.Present ||
                (p.PreviousAllocationId.Value != 0 && p.PreviousAllocationId.Value != p.AllocationId)) &&
            (p.Operation == 1 || !p.ActualBytes.Present || p.ActualBytes.Value >= p.RequestedBytes) &&
            (!p.BaselineCaptureId.Present || p.BaselineCaptureId.Value != GUID_NULL) && Valid(p.DataQuality) && Valid(p.ChangeCause));
    }
    HRESULT Write(const Emission& emission, const Allocation& p) noexcept
    {
        if (!InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::Allocation);
    }
    HRESULT Validate(const Loss& p) noexcept
    {
        return Result(p.StartQpc >= 0 && p.EndQpc >= p.StartQpc && (!p.LostEvents.Present || p.LostEvents.Value != 0) &&
            String(p.Reason, true) && (!p.HasDomains || ValidArray(p.Domains, 256, [](const DomainReference& value) noexcept { return Valid(value); })));
    }
    HRESULT Write(const Emission& emission, const Loss& p) noexcept
    {
        return WriteNative(emission, p, Event::Loss);
    }
    HRESULT Validate(const XamlPropertyChanged& p) noexcept
    {
        return Result(Valid(p.Object) && p.Object.IdentityDomain == Domain::XamlCore && p.PropertyId != 0 &&
            String(p.DeclaringType, true) && String(p.Name, true) && Valid(p.Previous) && Valid(p.Current) &&
            p.Previous.Unit == p.Current.Unit && Valid(p.ChangeCause));
    }
    HRESULT Validate(const RuntimeCapabilities& p) noexcept
    {
        uint32_t seen = 0;
        return Result(ValidArray(p.Capabilities, 128, [&](const CapabilityEntry& item) noexcept
        {
            if (item.Capability > 11 || item.Version < 1 || !Valid(item.DataQuality)) return false;
            const uint32_t bit = uint32_t{1} << item.Capability;
            if (seen & bit) return false;
            seen |= bit;
            return true;
        }));
    }
    HRESULT Write(const Emission& emission, const RuntimeCapabilities& p) noexcept
    {
        return WriteNative(emission, p, Event::RuntimeCapabilities);
    }
    HRESULT Validate(const CaptureBoundary& p) noexcept
    {
        if (p.CaptureId == GUID_NULL || p.Kind > 2 || p.Phase > 1 ||
            (p.RootId.Present && p.RootId.Value == GUID_NULL) || (p.DispatcherId.Present && p.DispatcherId.Value == 0) ||
            (p.Phase == 1 && !p.ChunkCount.Present) || !Valid(p.DataQuality) ||
            (p.DataQuality.Coverage == 0 && p.Domains.Count == 0)) return E_INVALIDARG;
        uint32_t seen = 0;
        return Result(ValidArray(p.Domains, 256, [&](const DomainRange& item) noexcept
        {
            if (!Valid(item.Domain) || !Valid(item.DataQuality)) return false;
            const uint32_t bit = uint32_t{1} << item.Domain.EventId;
            if (seen & bit) return false;
            seen |= bit;
            if (item.BeforeSequence.Present && item.ThroughSequence.Present && item.BeforeSequence.Value > item.ThroughSequence.Value)
                return false;
            if (item.Cut.Present && (item.Cut.Value.CutId == GUID_NULL || item.Cut.Value.SettledThrough > item.Cut.Value.ReservedThrough))
                return false;
            if (item.DataQuality.Coverage == 0)
            {
                if (!item.BeforeSequence.Present) return false;
                if (p.Phase == 1 && (!item.ThroughSequence.Present || !item.Cut.Present ||
                    item.Cut.Value.SettledThrough < item.ThroughSequence.Value)) return false;
            }
            return true;
        }));
    }
    HRESULT Write(const Emission& emission, const CaptureBoundary& p) noexcept
    {
        return WriteNative(emission, p, Event::CaptureBoundary);
    }
    HRESULT Validate(const CaptureChunk& p) noexcept
    {
        if (p.CaptureId == GUID_NULL || p.RootId == GUID_NULL || p.DispatcherId == 0) return E_INVALIDARG;
        const auto payload = [](const auto& item) noexcept { return SUCCEEDED(Validate(item)); };
        return Result(ValidArray(p.Objects, 4096, [](const SnapshotObject& item) noexcept { return Valid(item); }) &&
            ValidArray(p.Ownership, 8192, [](const OwnershipEdge& item) noexcept { return Valid(item); }) &&
            ValidArray(p.Tree, 4096, [&](const TreeEdge& item) noexcept
                { return SUCCEEDED(Validate(item)) && item.RootId == p.RootId && item.DispatcherId == p.DispatcherId; }) &&
            ValidArray(p.Correlations, 4096, payload) && ValidArray(p.Resources, 4096, payload) &&
            ValidArray(p.Visuals, 4096, payload) && ValidArray(p.Associations, 4096, payload) &&
            ValidArray(p.Types, 4096, payload) && ValidArray(p.Allocations, 4096, payload));
    }
    HRESULT Write(const Emission& emission, const CaptureChunk& p) noexcept
    {
        if (FAILED(Validate(p))) return E_INVALIDARG;
        const auto cause = [&](const Cause& value) noexcept { return InEpoch(value, emission); };
        const auto object = [&](const ObjectIdentity& value) noexcept { return InEpoch(value, emission); };
        if (!ValidArray(p.Objects, 4096, [&](const SnapshotObject& item) noexcept { return object(item.Key); }) ||
            !ValidArray(p.Ownership, 8192, [&](const OwnershipEdge& item) noexcept { return object(item.From) && object(item.To); }) ||
            !ValidArray(p.Tree, 4096, [&](const TreeEdge& item) noexcept
                { return object(item.Child) && (!item.Parent.Present || object(item.Parent.Value)) && cause(item.ChangeCause); }) ||
            !ValidArray(p.Correlations, 4096, [&](const Correlation& item) noexcept
                { return object(item.From) && object(item.To) && cause(item.ChangeCause); }) ||
            !ValidArray(p.Resources, 4096, [&](const Resource& item) noexcept
                { return object(item.Object) && (!item.Owner.Present || object(item.Owner.Value)) && cause(item.ChangeCause); }) ||
            !ValidArray(p.Visuals, 4096, [&](const VisualDelta& item) noexcept { return object(item.Visual) && cause(item.ChangeCause); }) ||
            !ValidArray(p.Associations, 4096, [&](const CorePeerAssociation& item) noexcept
                { return object(item.Core) && object(item.Peer) && cause(item.ChangeCause); }) ||
            !ValidArray(p.Allocations, 4096, [&](const Allocation& item) noexcept { return cause(item.ChangeCause); })) return E_INVALIDARG;
        return WriteNative(emission, p, Event::CaptureChunk);
    }
    HRESULT Write(const Emission& emission, const XamlPropertyChanged& p) noexcept
    {
        if (!InEpoch(p.Object, emission) || !InEpoch(p.ChangeCause, emission)) return E_INVALIDARG;
        return WriteNative(emission, p, Event::XamlPropertyChanged);
    }
}
