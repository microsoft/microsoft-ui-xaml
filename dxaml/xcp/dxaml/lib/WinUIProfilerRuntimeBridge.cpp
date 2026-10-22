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
        thread_local bool s_resolvingEligibility = false;
        SRWLOCK s_eligibilityLock = SRWLOCK_INIT;
        volatile LONG64 s_cachedEligibilitySerial = -1;
        volatile LONG64 s_enabledDefinitions = 0;
        volatile LONG64 s_fastDefinitions = 0;

        SRWLOCK s_listenerLock = SRWLOCK_INIT;
        bool s_anyListener = false;
        volatile LONG s_providerFault = 0;
        REGHANDLE s_providerHandle = 0;
        constexpr char ProviderName[] = "WinUIProfiler-Runtime";
        BYTE s_providerTraits[sizeof(ProviderName) + 2]{};
        uint64_t EventKeyword(Event event) noexcept;

        void NTAPI OnEnable(LPCGUID, ULONG control, UCHAR level, ULONGLONG keywords, ULONGLONG,
            PEVENT_FILTER_DESCRIPTOR, PVOID) noexcept
        {
            // Notification only: no COM, GUID creation, allocation or dispatcher work.
            AcquireSRWLockExclusive(&s_listenerLock);
            if (control == EVENT_CONTROL_CODE_ENABLE_PROVIDER)
            {
                // ETW notifications describe aggregate provider state, not an
                // authoritative per-listener eligibility/receipt table.
                if (!s_anyListener)
                {
                    s_anyListener = true;
                    InterlockedIncrement64(&s_enableGeneration);
                }
                uint64_t mask = 0;
                for (uint16_t id = 1; id <= 16; ++id)
                {
                    const auto definition = static_cast<Event>(id);
                    if ((keywords & EventKeyword(definition)) && (level == 0 || level >= (definition == Event::Loss ? 3 : 4)))
                        mask |= uint64_t{1} << id;
                }
                InterlockedExchange64(&s_fastDefinitions, static_cast<LONG64>(mask));
            }
            else if (control == EVENT_CONTROL_CODE_DISABLE_PROVIDER)
            {
                s_anyListener = false;
                InterlockedExchange64(&s_fastDefinitions, 0);
            }
            InterlockedIncrement64(&s_capabilitiesSerial);
            ReleaseSRWLockExclusive(&s_listenerLock);
        }

        struct Registration
        {
            Registration() noexcept
            {
                s_providerTraits[0] = static_cast<BYTE>(sizeof(s_providerTraits));
                s_providerTraits[1] = static_cast<BYTE>(sizeof(s_providerTraits) >> 8);
                memcpy(s_providerTraits + 2, ProviderName, sizeof(ProviderName));
                ULONG result = EventRegister(&RuntimeProviderId, OnEnable, nullptr, &s_providerHandle);
                if (result == ERROR_SUCCESS)
                    result = EventSetInformation(s_providerHandle, EventProviderSetTraits, s_providerTraits, sizeof(s_providerTraits));
                if (result != ERROR_SUCCESS) InterlockedExchange(&s_providerFault, result);
            }
            ~Registration() { if (s_providerHandle) EventUnregister(s_providerHandle); }
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

        ULONG QueryDefinitionSelections(uint64_t* mask) noexcept
        {
            // SDK/EventEnabled checks can combine one session's level with
            // another's keywords. Query actual selections outside callbacks.
            constexpr ULONG bufferSize = 64 * 1024;
            struct Scratch
            {
                BYTE* Data;
                ~Scratch() { if (Data) HeapFree(GetProcessHeap(), 0, Data); }
            } scratch{static_cast<BYTE*>(HeapAlloc(GetProcessHeap(), 0, bufferSize))};
            if (!scratch.Data) return ERROR_NOT_ENOUGH_MEMORY;
            BYTE* buffer = scratch.Data;
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

        bool DefinitionEnabled(Event event) noexcept
        {
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
                EventKeyword(definition), definition == Event::Loss ? 3 : 4);
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
        s_insideInstrumentation = true;
        return true;
    }
    void LeaveInstrumentation() noexcept { s_insideInstrumentation = false; }
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
        return Result(p.DeviceEpoch != GUID_NULL && p.Operation <= 2 && String(p.Api, true) && Valid(p.DataQuality));
    }
    HRESULT Write(const Emission& emission, const Device& p) noexcept
    {
        return WriteNative(emission, p, Event::Device);
    }
    HRESULT Validate(const Resource& p) noexcept
    {
        return Result(Valid(p.Object) && p.Object.IdentityDomain == Domain::Resource && p.Operation <= 6 &&
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
