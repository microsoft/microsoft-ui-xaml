// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerRuntimeBridge.h"

#if !defined(WINUI_PROFILER_ENABLED) || WINUI_PROFILER_ENABLED != 1
#error WinUI profiler foundation is required in fre and chk.
#endif

namespace WinUIProfiler
{
    template<typename T> struct Optional { bool Present = false; T Value{}; };
    template<typename T> struct Array { const T* Items = nullptr; uint32_t Count = 0; };
    struct Quality { uint8_t Kind = 0; uint8_t Coverage = 0; const char* Reason = nullptr; };
    struct Classification { uint8_t Kind = 0; uint8_t Evidence = 0; const char* Source = "not observed"; };
    struct ObjectIdentity { Domain IdentityDomain = Domain::XamlCore; GUID Epoch{}; uint64_t Id = 0; };
    struct Attempt { GUID Epoch{}; uint64_t Id = 0; GUID RootId{}; uint64_t DispatcherId = 0; };
    struct Cause { uint8_t Kind = 0; Optional<Attempt> Reference{}; const char* Reason = "not observed"; };
    struct Value
    {
        uint8_t Kind = 4;
        uint8_t Unit = 0;
        int64_t Integer = 0;
        double Number = 0;
        bool Boolean = false;
        const char* Text = nullptr;
        Quality DataQuality{2, 2, "not observed"};
    };

    struct ObjectLifetime
    {
        ObjectIdentity Object;
        uint8_t Operation = 0;
        Optional<uint64_t> TypeId, Address;
        Quality DataQuality;
        Optional<Classification> TypeClassification;
        Cause ChangeCause;
    };
    struct TypeResolved { uint64_t TypeId = 0; const char* Name = nullptr; Optional<Classification> TypeClassification; };
    struct CorePeerAssociation { ObjectIdentity Core, Peer; bool Linked = false; Cause ChangeCause; };
    struct TreeEdge
    {
        Optional<ObjectIdentity> Parent;
        ObjectIdentity Child;
        uint8_t Relationship = 0;
        int32_t Order = 0;
        GUID RootId{};
        uint64_t DispatcherId = 0;
        bool Connected = false;
        Cause ChangeCause;
        uint8_t Membership = 0;
    };
    struct VisualDelta { ObjectIdentity Visual; uint8_t Property = 0; Value PropertyValue; Cause ChangeCause; };
    struct Correlation { ObjectIdentity From, To; uint8_t Kind = 0; bool Linked = false; Cause ChangeCause; };
    struct Boundary { uint8_t Kind = 0; uint8_t Phase = 0; uint64_t OperationId = 0; Optional<int32_t> HResult; Cause ChangeCause; };
    struct Frame { Optional<Attempt> Reference; Optional<uint64_t> SubmittedFrameId; uint8_t Outcome = 0; Optional<int32_t> HResult; };
    struct Device { GUID DeviceEpoch{}; uint8_t Operation = 0; const char* Api = nullptr; Quality DataQuality; };
    struct Resource
    {
        ObjectIdentity Object;
        uint8_t Operation = 0;
        GUID DeviceEpoch{};
        Optional<GUID> BackingAllocationId;
        Optional<uint64_t> AllocationBytes;
        uint8_t SizeProvenance = 2;
        Optional<ObjectIdentity> Owner;
        Optional<uint32_t> WidthPixels, HeightPixels;
        Quality DataQuality;
        Classification ResourceClassification;
        Cause ChangeCause;
    };
    struct Allocation
    {
        uint64_t AllocationId = 0;
        uint8_t Operation = 0;
        Optional<uint64_t> PreviousAllocationId;
        uint64_t RequestedBytes = 0;
        Optional<uint64_t> ActualBytes;
        GUID AllocatorEpoch{};
        Optional<GUID> BaselineCaptureId;
        Quality DataQuality;
        Cause ChangeCause;
    };
    struct CapabilityEntry { uint8_t Capability = 0; int32_t Version = 1; Quality DataQuality; };
    struct RuntimeCapabilities { Array<CapabilityEntry> Capabilities; };
    // Runtime vector domains inherit the common provider/process/epoch.
    struct DomainReference { uint16_t EventId = 0; uint8_t Version = EventVersion; };
    struct SettledCut { GUID CutId{}; uint64_t ReservedThrough = 0; uint64_t SettledThrough = 0; };
    struct DomainRange
    {
        DomainReference Domain;
        Optional<uint64_t> BeforeSequence, ThroughSequence;
        Quality DataQuality;
        Optional<SettledCut> Cut;
    };
    struct CaptureBoundary
    {
        GUID CaptureId{};
        uint8_t Kind = 0;
        uint8_t Phase = 0;
        Optional<GUID> RootId;
        Optional<uint64_t> DispatcherId;
        Array<DomainRange> Domains;
        Optional<uint32_t> ChunkCount;
        Quality DataQuality;
    };
    struct SnapshotObject
    {
        ObjectIdentity Key;
        const char* TypeName = nullptr;
        Optional<uint64_t> Address;
        Value Size{4, 1, 0, 0, false, nullptr, {2, 2, "not observed"}};
        Optional<Classification> TypeClassification;
    };
    struct OwnershipEdge
    {
        ObjectIdentity From, To;
        const char* Relationship = nullptr;
        uint8_t Evidence = 0;
        Quality DataQuality;
    };
    struct CaptureChunk
    {
        GUID CaptureId{};
        uint32_t ChunkIndex = 0;
        GUID RootId{};
        uint64_t DispatcherId = 0;
        Array<SnapshotObject> Objects;
        Array<OwnershipEdge> Ownership;
        Array<TreeEdge> Tree;
        Array<Correlation> Correlations;
        Array<Resource> Resources;
        Array<VisualDelta> Visuals;
        Array<CorePeerAssociation> Associations;
        Array<TypeResolved> Types;
        Array<Allocation> Allocations;
    };
    struct Loss
    {
        int64_t StartQpc = 0;
        int64_t EndQpc = 0;
        Optional<uint64_t> LostEvents;
        const char* Reason = nullptr;
        bool HasDomains = false;
        Array<DomainReference> Domains;
    };
    struct XamlPropertyChanged
    {
        ObjectIdentity Object;
        uint64_t PropertyId = 0;
        const char* DeclaringType = nullptr;
        const char* Name = nullptr;
        Value Previous, Current;
        Cause ChangeCause;
    };

    struct Emission
    {
        ProtocolInfo Info{};
        SequenceDomain* Sequence = nullptr;
        uint64_t SourceSequence = 0;
        Event Definition{};
        bool OwnsInstrumentation = false;
    };
    bool IsEnabled(Event event) noexcept;
    HRESULT BeginEmission(Event event, Emission* emission) noexcept;
    void EndEmission(Emission* emission) noexcept;
    HRESULT NewObjectIdentity(Domain domain, ObjectIdentity* identity) noexcept;

    template<Event> struct EventTraits;
#define WINUI_PROFILER_EVENT(name) \
    template<> struct EventTraits<Event::name> { using Payload = name; }; \
    HRESULT Write(const Emission& emission, const name& payload) noexcept; \
    HRESULT Validate(const name& payload) noexcept;
    WINUI_PROFILER_EVENT(RuntimeCapabilities)
    WINUI_PROFILER_EVENT(ObjectLifetime)
    WINUI_PROFILER_EVENT(TypeResolved)
    WINUI_PROFILER_EVENT(CorePeerAssociation)
    WINUI_PROFILER_EVENT(TreeEdge)
    WINUI_PROFILER_EVENT(VisualDelta)
    WINUI_PROFILER_EVENT(Correlation)
    WINUI_PROFILER_EVENT(Boundary)
    WINUI_PROFILER_EVENT(Frame)
    WINUI_PROFILER_EVENT(Device)
    WINUI_PROFILER_EVENT(Resource)
    WINUI_PROFILER_EVENT(Allocation)
    WINUI_PROFILER_EVENT(CaptureBoundary)
    WINUI_PROFILER_EVENT(CaptureChunk)
    WINUI_PROFILER_EVENT(Loss)
    WINUI_PROFILER_EVENT(XamlPropertyChanged)
#undef WINUI_PROFILER_EVENT

    template<Event Definition, typename Factory>
    HRESULT Trace(Factory&& factory) noexcept
    {
        static_assert(noexcept(factory()), "Instrumentation factories must not affect application exception behavior.");
        if (!IsEnabled(Definition) || !EnterInstrumentation()) return S_FALSE;
        struct Guard { ~Guard() { LeaveInstrumentation(); } } guard;
        const typename EventTraits<Definition>::Payload payload = factory();
        HRESULT result = Validate(payload);
        if (FAILED(result)) return result;
        Emission emission;
        result = BeginEmission(Definition, &emission);
        if (result != S_OK) return result;
        result = Write(emission, payload);
        EndEmission(&emission);
        return result;
    }
}
