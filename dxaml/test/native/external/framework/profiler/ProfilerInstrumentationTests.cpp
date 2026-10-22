// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "ProfilerInstrumentationTests.h"
#include "WinUIProfilerRuntimeBridge.h"
#include "WinUIProfilerTracing.h"
#include <evntrace.h>
#include <tdh.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <initializer_list>
#include <thread>
#include <atomic>
#include <cstring>
#include <limits>

using namespace WinUIProfiler;

namespace
{
    void Check(bool condition, const char* name)
    {
        if (!condition)
        {
            std::printf("FAIL: %s\n", name);
#ifdef WINUI_PROFILER_STANDALONE
            throw std::runtime_error(name);
#else
            VERIFY_IS_TRUE(condition);
#endif
        }
    }

    void ProtocolTest()
    {
        ProtocolInfo info{};
        Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK, "v1 protocol handshake");
        Check(info.Version == 1 && info.Size == sizeof(info), "explicit ABI size/version");
        Check(info.Process.ProcessId == GetCurrentProcessId(), "actual target process");
        Check(info.Process.HasCreationTime == 1 && info.Process.CreationTimeFileTime > 0, "actual process creation time");
        Check(info.ProducerEpoch != GUID_NULL && info.Process.InstanceId != GUID_NULL, "qualified producer/process identities");
        Check(info.QpcFrequency > 0 && info.CapabilityCount == 0, "no fabricated later capabilities");
        Check(GetProtocolInfo(2, sizeof(info), &info) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "reject incompatible version");
        Check(GetProtocolInfo(1, sizeof(info) - 1, &info) == E_INVALIDARG, "reject wrong ABI size");
        Check(GetProtocolInfo(1, sizeof(info), nullptr) == E_POINTER, "reject null ABI buffer");
        Check(RequestCapture(1, 0, info.ProducerEpoch) == E_NOTIMPL, "future capture explicitly unsupported");
        Check(RequestCapture(2, 0, info.ProducerEpoch) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "capture rejects incompatible version");
        std::puts("PASS: protocol");
    }

    void SequenceTest()
    {
        SequenceDomain domain;
        uint64_t sequences[7]{};
        for (auto& sequence : sequences) Check(domain.Reserve(&sequence) == S_OK, "reserve emission attempt");
        Check(sequences[0] == 1 && sequences[6] == 7, "reservations start at one");
        for (uint64_t i = 1; i <= 4; ++i) Check(domain.Settle(i) == S_OK, "settle prefix");
        Check(domain.Settle(7) == S_OK, "out-of-order terminal write");
        uint64_t reserved, settled;
        domain.Snapshot(&reserved, &settled);
        Check(reserved == 7 && settled == 4, "max completion does not settle in-flight 5/6");
        Check(domain.Settle(5) == S_OK, "settle delayed 5");
        domain.Snapshot(&reserved, &settled);
        Check(settled == 5, "6 still in flight");
        Check(domain.Settle(6) == S_OK, "failed writes also settle");
        domain.Snapshot(&reserved, &settled);
        Check(settled == 7, "contiguous prefix advances across completed 7");
        Check(domain.Settle(7) == E_INVALIDARG && domain.Settle(8) == E_INVALIDARG, "reject duplicate/unreserved completion");
        SequenceDomain bounded;
        for (uint32_t i = 0; i < SequenceDomain::Window; ++i)
            Check(bounded.Reserve(&reserved) == S_OK, "fill bounded in-flight window");
        Check(bounded.Reserve(&reserved) == HRESULT_FROM_WIN32(ERROR_BUSY), "bounded reservation backpressure");
        Check(bounded.Settle(1) == S_OK && bounded.Reserve(&reserved) == S_OK, "reuse settled reservation slot");
        SequenceDomain concurrent;
        std::atomic<bool> valid{true};
        std::vector<std::thread> workers;
        for (int i = 0; i < 8; ++i)
        {
            workers.emplace_back([&]()
            {
                for (int j = 0; j < 512; ++j)
                {
                    uint64_t sequence;
                    HRESULT result;
                    do { result = concurrent.Reserve(&sequence); } while (result == HRESULT_FROM_WIN32(ERROR_BUSY));
                    if (result != S_OK || concurrent.Settle(sequence) != S_OK) valid = false;
                    uint64_t r, s;
                    concurrent.Snapshot(&r, &s);
                    if (s > r) valid = false;
                }
            });
        }
        for (auto& worker : workers) worker.join();
        concurrent.Snapshot(&reserved, &settled);
        Check(valid && reserved == 4096 && settled == reserved, "concurrent reservations have a contiguous terminal prefix");
        std::puts("PASS: contiguous settlement");
    }

    class Session
    {
    public:
        Session()
        {
            static volatile LONG serial = 0;
            swprintf_s(m_name, L"WinUIProfilerFoundation-%lu-%llu-%ld",
                GetCurrentProcessId(), GetTickCount64(), InterlockedIncrement(&serial));
            m_properties.Properties.Wnode.BufferSize = sizeof(m_properties);
            m_properties.Properties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
            m_properties.Properties.Wnode.ClientContext = 1;
            m_properties.Properties.LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
            m_properties.Properties.LoggerNameOffset = offsetof(SessionProperties, Name);
            m_properties.Properties.LogFileNameOffset = offsetof(SessionProperties, File);
            wchar_t relative[256];
            swprintf_s(relative, L"BuildOutput\\ProfilerHarness\\%s.etl", m_name);
            Check(GetFullPathNameW(relative, MAX_PATH, m_properties.File, nullptr) != 0, "local ETL path");
            const ULONG result = StartTraceW(&m_handle, m_name, &m_properties.Properties);
            if (result) std::printf("StartTrace(%ls) failed: %lu\n", m_name, result);
            Check(result == ERROR_SUCCESS, "start isolated ETW session");
        }
        ~Session()
        {
            if (m_handle) ControlTraceW(m_handle, m_name, &m_properties.Properties, EVENT_TRACE_CONTROL_STOP);
            DeleteFileW(m_properties.File);
        }
        void Enable(uint64_t keywords, UCHAR level = 4, uint64_t matchAll = 0)
        {
            Check(EnableTraceEx2(m_handle, &RuntimeProviderId, EVENT_CONTROL_CODE_ENABLE_PROVIDER, level,
                keywords, matchAll, 5000, nullptr) == ERROR_SUCCESS, "enable actual native provider");
        }
        void Disable()
        {
            Check(EnableTraceEx2(m_handle, &RuntimeProviderId, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0,
                0, 0, 5000, nullptr) == ERROR_SUCCESS, "disable only this listener");
        }
        void Read(PEVENT_RECORD_CALLBACK callback, void* context)
        {
            Check(ControlTraceW(m_handle, m_name, &m_properties.Properties, EVENT_TRACE_CONTROL_STOP) == ERROR_SUCCESS, "stop owned test transport");
            m_handle = 0;
            EVENT_TRACE_LOGFILEW log{};
            log.LogFileName = m_properties.File;
            log.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
            log.EventRecordCallback = callback;
            log.Context = context;
            TRACEHANDLE reader = OpenTraceW(&log);
            Check(reader != INVALID_PROCESSTRACE_HANDLE, "open actual ETL");
            const ULONG result = ProcessTrace(&reader, 1, nullptr, nullptr);
            CloseTrace(reader);
            Check(result == ERROR_SUCCESS, "enumerate actual ETL");
            Check(DeleteFileW(m_properties.File) != FALSE, "remove owned ETL");
        }
    private:
        struct SessionProperties { EVENT_TRACE_PROPERTIES Properties{}; wchar_t Name[128]{}; wchar_t File[MAX_PATH]{}; } m_properties;
        TRACEHANDLE m_handle = 0;
        wchar_t m_name[128]{};
    };

    void QualityCrossProductTest()
    {
        XamlPropertyChanged property;
        property.Object = {Domain::XamlCore, RuntimeProviderId, 1};
        property.PropertyId = 1;
        property.DeclaringType = "Element";
        property.Name = "Test";
        for (uint8_t kind = 0; kind < 4; ++kind)
        {
            Value present;
            present.Kind = kind;
            present.Unit = kind == 2 ? 5 : kind == 3 ? 7 : 0;
            present.Text = kind == 3 ? "value" : nullptr;
            property.Previous = present;
            property.Current = present;
            Check(Validate(property) == E_INVALIDARG, "each present kind rejects unavailable measurement quality");
        }
        for (uint8_t coverage = 0; coverage < 3; ++coverage)
        {
            property.Previous = {};
            property.Current = {};
            property.Previous.DataQuality = {2, coverage, "unavailable measurement"};
            property.Current.DataQuality = property.Previous.DataQuality;
            Check(Validate(property) == S_OK, "missing measurement preserves independent Complete/Partial/Unavailable coverage");
            property.Current.DataQuality.Reason = nullptr;
            Check(Validate(property) == E_INVALIDARG, "missing measurement still requires a reason");
        }
        std::puts("PASS: typed-value quality/discriminator cross-product");
    }

    struct IndependentDecoded { uint16_t EventId; GUID Epoch; uint32_t Count = 0; bool Failed = false; };
    void WINAPI OnIndependentEvent(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId) return;
        auto& state = *static_cast<IndependentDecoded*>(record->UserContext);
        PROPERTY_DATA_DESCRIPTOR field{};
        field.ArrayIndex = ULONG_MAX;
        field.PropertyName = reinterpret_cast<ULONGLONG>(L"SemanticEventId");
        uint16_t eventId = 0;
        ULONG result = TdhGetProperty(record, 0, nullptr, 1, &field, sizeof(eventId), reinterpret_cast<BYTE*>(&eventId));
        field.PropertyName = reinterpret_cast<ULONGLONG>(L"ProducerEpoch");
        GUID epoch{};
        result |= TdhGetProperty(record, 0, nullptr, 1, &field, sizeof(epoch), reinterpret_cast<BYTE*>(&epoch));
        if (result != ERROR_SUCCESS || eventId != state.EventId || epoch != state.Epoch ||
            record->EventHeader.ProcessId != GetCurrentProcessId() ||
            record->EventHeader.EventDescriptor.Level != (eventId == 15 ? 3 : 4)) state.Failed = true;
        ++state.Count;
    }

    void IndependentEpochTest()
    {
        {
            Session frames;
            frames.Enable(16);
            Check(IsEnabled(Event::Frame), "Frames-only listener enables its definition");
            Check(!IsEnabled(Event::RuntimeCapabilities), "Frames-only test has no capabilities prerequisite");
            Check(Trace<Event::Frame>([]() noexcept { Frame value; value.Outcome = 3; return value; }) == S_OK,
                "fresh Frames-only epoch writes without a capabilities publication gate");
            SequenceCut cut;
            Check(GetSequenceCut(1, 9, sizeof(cut), &cut) == S_OK && cut.ReservedThrough == 1 &&
                cut.SettledThrough == 1, "Frames-only reservation settles in its own epoch");
            IndependentDecoded decoded{9, cut.ProducerEpoch};
            frames.Read(OnIndependentEvent, &decoded);
            Check(!decoded.Failed && decoded.Count == 1, "TDH actually receives the fresh Frames-only record");
        }
        {
            Session loss;
            loss.Enable(1, 3);
            Check(IsEnabled(Event::Loss) && !IsEnabled(Event::RuntimeCapabilities), "Warning-only listener selects Loss, not informational capabilities");
            Check(Trace<Event::Loss>([]() noexcept { Loss value; value.Reason = "test loss"; return value; }) == S_OK,
                "fresh Warning-only Loss epoch writes independently");
            SequenceCut cut;
            Check(GetSequenceCut(1, 15, sizeof(cut), &cut) == S_OK && cut.ReservedThrough == 1 &&
                cut.SettledThrough == 1, "Warning-only Loss reservation settles independently");
            IndependentDecoded decoded{15, cut.ProducerEpoch};
            loss.Read(OnIndependentEvent, &decoded);
            Check(!decoded.Failed && decoded.Count == 1, "TDH actually receives the Warning-only Loss record without informational enablement");
        }
        std::puts("PASS: independently enabled fresh Frames-only and Warning-only Loss epochs");
    }

    void EnabledTest()
    {
        int expensive = 0;
        auto build = [&]() noexcept { ++expensive; Frame frame; frame.Outcome = 3; return frame; };
        Check(Trace<Event::Frame>(build) == S_FALSE && expensive == 0, "disabled guard precedes argument evaluation");
        Session session;
        session.Enable(static_cast<uint64_t>(Keyword::Frames) | static_cast<uint64_t>(Keyword::Capabilities));
        Check(IsEnabled(Event::Frame), "fre provider exposes enabled frame definition");
        Check(!IsEnabled(Event::ObjectLifetime), "independent keyword families");
        Check(Trace<Event::Frame>(build) == S_OK && expensive == 1, "enabled native typed frame write");
        std::puts("PASS: enabled/disabled native provider");
    }

    template<typename T>
    T Field(EVENT_RECORD* record, std::initializer_list<const wchar_t*> path, ULONG arrayIndex = ULONG_MAX)
    {
        std::vector<PROPERTY_DATA_DESCRIPTOR> descriptors;
        for (const wchar_t* name : path)
        {
            PROPERTY_DATA_DESCRIPTOR descriptor{};
            descriptor.PropertyName = reinterpret_cast<ULONGLONG>(name);
            descriptor.ArrayIndex = descriptors.empty() ? arrayIndex : ULONG_MAX;
            descriptors.push_back(descriptor);
        }
        T value{};
        ULONG size = 0;
        const ULONG status = TdhGetPropertySize(record, 0, nullptr, static_cast<ULONG>(descriptors.size()), descriptors.data(), &size);
        if (status != ERROR_SUCCESS)
        {
            std::printf("TDH property status=%lu arrayIndex=%lu path:", status, arrayIndex);
            for (const wchar_t* name : path) std::printf(" %ls", name);
            std::puts("");
        }
        Check(status == ERROR_SUCCESS, "TDH typed property exists");
        Check(size == sizeof(T), "TDH fixed-width property type");
        Check(TdhGetProperty(record, 0, nullptr, static_cast<ULONG>(descriptors.size()), descriptors.data(),
            sizeof(value), reinterpret_cast<BYTE*>(&value)) == ERROR_SUCCESS, "TDH actual property value");
        return value;
    }

    struct Decoded { bool Failed = false; uint32_t Counts[17]{}; };
    void WireType(const TRACE_EVENT_INFO* info, const BYTE* metadata,
        std::initializer_list<const wchar_t*> path, USHORT input, USHORT output = 0)
    {
        ULONG first = 0;
        ULONG count = info->TopLevelPropertyCount;
        size_t remaining = path.size();
        for (const wchar_t* name : path)
        {
            const EVENT_PROPERTY_INFO* found = nullptr;
            for (ULONG i = first; i < first + count; ++i)
            {
                Check(i < info->PropertyCount, "TDH struct member bounds");
                const auto& property = info->EventPropertyInfoArray[i];
                if (wcscmp(reinterpret_cast<const wchar_t*>(metadata + property.NameOffset), name) == 0) found = &property;
            }
            Check(found != nullptr, "typed metadata property exists");
            if (--remaining)
            {
                Check((found->Flags & PropertyStruct) != 0, "wire nesting uses TDH structs, not opaque binary");
                first = found->structType.StructStartIndex;
                count = found->structType.NumOfStructMembers;
            }
            else Check((found->Flags & PropertyStruct) == 0 && found->nonStructType.InType == input &&
                found->nonStructType.OutType == output, "exact TDH primitive input/output type");
        }
    }
    void WINAPI OnEvent(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId) return;
        auto& decoded = *static_cast<Decoded*>(record->UserContext);
        try
        {
            Check(record->UserDataLength <= MaximumWireBytes, "ETW wire budget independent of normalized JSON budget");
            Check(record->EventHeader.ProcessId == GetCurrentProcessId(), "ETW source PID");
            Check(Field<uint32_t>(record, {L"TargetProcess", L"ProcessId"}) == GetCurrentProcessId(), "semantic target PID");
            Check(Field<uint8_t>(record, {L"EventSchemaVersion"}) == 1, "explicit schema version");
            Check(Field<GUID>(record, {L"ProducerEpoch"}) != GUID_NULL, "actual wire epoch");
            Check(Field<uint64_t>(record, {L"SourceSequence"}) > 0, "actual wire reservation");
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            Check(id > 0 && id <= 16 && id != 13 && id != 14, "only implemented semantic definitions");
            ++decoded.Counts[id];
            const wchar_t* names[] = {L"", L"RuntimeCapabilities", L"ObjectLifetime", L"TypeResolved", L"CorePeerAssociation",
                L"TreeEdge", L"VisualDelta", L"Correlation", L"Boundary", L"Frame", L"Device", L"Resource", L"Allocation",
                L"", L"", L"Loss", L"XamlPropertyChanged"};
            const uint64_t keywords[] = {0, 1, 2, 2, 2, 4, 8, 8, 16, 16, 32, 32, 64, 0, 0, 1, 2};
            ULONG size = 0;
            Check(TdhGetEventInformation(record, 0, nullptr, nullptr, &size) == ERROR_INSUFFICIENT_BUFFER, "TraceLogging self-describing metadata");
            std::vector<BYTE> metadata(size);
            auto info = reinterpret_cast<TRACE_EVENT_INFO*>(metadata.data());
            Check(TdhGetEventInformation(record, 0, nullptr, info, &size) == ERROR_SUCCESS, "decode actual native schema metadata");
            WireType(info, metadata.data(), {L"SemanticEventId"}, TDH_INTYPE_UINT16);
            WireType(info, metadata.data(), {L"EventSchemaVersion"}, TDH_INTYPE_UINT8);
            WireType(info, metadata.data(), {L"SourceSequence"}, TDH_INTYPE_UINT64);
            WireType(info, metadata.data(), {L"ProducerEpoch"}, TDH_INTYPE_GUID);
            WireType(info, metadata.data(), {L"TargetProcess", L"ProcessId"}, TDH_INTYPE_UINT32);
            WireType(info, metadata.data(), {L"TargetProcess", L"CreationTimeFileTime"}, TDH_INTYPE_INT64);
            WireType(info, metadata.data(), {L"TargetProcess", L"HasCreationTimeFileTime"}, TDH_INTYPE_UINT8, TDH_OUTTYPE_BOOLEAN);
            Check(info->EventNameOffset != 0 && wcscmp(reinterpret_cast<wchar_t*>(metadata.data() + info->EventNameOffset), names[id]) == 0,
                "event name matches semantic catalog ID, not numeric TraceLogging ID");
            Check(record->EventHeader.EventDescriptor.Keyword == keywords[id], "independent keyword mapping");
            Check(record->EventHeader.EventDescriptor.Level == (id == 15 ? 3 : 4), "catalog event levels");
            if (id == 1)
            {
                Check(Field<uint32_t>(record, {L"CapabilityCount"}) == 0, "wire advertises no future hook/capture capabilities");
            }
            else if (id == 9)
            {
                Check(record->EventHeader.EventDescriptor.Keyword == 16, "frame keyword");
                Check(Field<uint8_t>(record, {L"Outcome"}) == 3, "typed standalone outcome");
                Check(Field<uint8_t>(record, {L"Attempt", L"Present"}) == 0, "standalone has no fabricated attempt");
                Check(Field<uint8_t>(record, {L"SubmittedFrameId", L"Present"}) == 0, "standalone has no fabricated submission");
                Check(Field<uint8_t>(record, {L"HResult", L"Present"}) == 0, "nullable result is explicit");
            }
            else if (id == 2)
            {
                Check(Field<uint64_t>(record, {L"Object", L"RuntimeObjectId"}) == 1, "lifetime is explicit ID, not pointer");
                Check(Field<uint8_t>(record, {L"Classification", L"Value", L"Kind"}) == 2, "authoritative UIElement classification");
                Check(Field<uint8_t>(record, {L"Address", L"Present"}) == 0, "address is nullable metadata");
            }
            else if (id == 3) Check(Field<uint64_t>(record, {L"TypeId"}) == 42, "epoch-scoped type ID");
            else if (id == 4)
            {
                Check(Field<uint8_t>(record, {L"Core", L"Domain"}) == 0 && Field<uint8_t>(record, {L"Peer", L"Domain"}) == 1, "separate core/peer identity domains");
                Check(Field<uint8_t>(record, {L"Linked"}) == 1, "association boolean");
            }
            else if (id == 5)
            {
                WireType(info, metadata.data(), {L"Order"}, TDH_INTYPE_INT32);
                Check(Field<uint8_t>(record, {L"Parent", L"Present"}) == 0, "root has no parent");
                Check(Field<uint8_t>(record, {L"Membership"}) == 2 && Field<uint64_t>(record, {L"DispatcherId"}) == 7, "visual membership/dispatcher");
            }
            else if (id == 6)
            {
                WireType(info, metadata.data(), {L"Value", L"Number"}, TDH_INTYPE_DOUBLE);
                Check(Field<uint8_t>(record, {L"Value", L"Kind"}) == 1 && Field<uint8_t>(record, {L"Value", L"Unit"}) == 4, "typed numeric visual property in pixels");
                Check(Field<double>(record, {L"Value", L"Number"}) == 3.5, "native f64 visual property");
            }
            else if (id == 7) Check(Field<uint8_t>(record, {L"To", L"Domain"}) == 2, "comp-node is not a canonical visual");
            else if (id == 8)
            {
                WireType(info, metadata.data(), {L"HResult", L"Value"}, TDH_INTYPE_INT32);
                Check(Field<uint64_t>(record, {L"OperationId"}) == 7, "boundary operation identity");
            }
            else if (id == 10)
            {
                WireType(info, metadata.data(), {L"Api"}, TDH_INTYPE_ANSISTRING, TDH_OUTTYPE_UTF8);
                Check(Field<GUID>(record, {L"DeviceEpoch"}) != GUID_NULL, "device epoch independent field");
            }
            else if (id == 11)
            {
                Check(Field<uint8_t>(record, {L"AllocationBytes", L"Present"}) == 0, "unknown resource size is not zero");
                Check(Field<uint8_t>(record, {L"Classification", L"Kind"}) == 1 && Field<uint8_t>(record, {L"Classification", L"Evidence"}) == 1, "creation API resource evidence");
                Check(Field<uint32_t>(record, {L"WidthPixels", L"Value"}) == 64, "resource dimensions are u32 pixels");
            }
            else if (id == 12)
            {
                Check(Field<uint64_t>(record, {L"RequestedBytes"}) == 64 && Field<uint8_t>(record, {L"ActualBytes", L"Present"}) == 0, "requested bytes do not fabricate actual allocator bytes");
            }
            else if (id == 15) Check(Field<uint8_t>(record, {L"HasDomains"}) == 0, "loss explicitly unlocalized, no invented listener proof");
            else if (id == 16)
            {
                Check(Field<uint64_t>(record, {L"Property", L"PropertyId"}) == 37, "authoritative property ID");
                Check(Field<uint8_t>(record, {L"Previous", L"Kind"}) == 4, "previous property value explicitly missing");
                Check(Field<double>(record, {L"Current", L"Number"}) == 37.5, "native typed current property value");
                Check(Field<uint64_t>(record, {L"Cause", L"Attempt", L"Value", L"AttemptId"}) == 9, "qualified causal attempt");
            }
        }
        catch (...) { decoded.Failed = true; }
    }

    void SchemaTest()
    {
        Session session;
        session.Enable(17);
        Check(Trace<Event::Frame>([]() noexcept { Frame frame; frame.Outcome = 3; return frame; }) == S_OK, "actual frame schema emission");
        session.Enable(127);
        ProtocolInfo info;
        Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK, "schema epoch handshake");
        ObjectIdentity core, peer, node, visual, resource;
        Check(NewObjectIdentity(Domain::XamlCore, &core) == S_OK && NewObjectIdentity(Domain::XamlPeer, &peer) == S_OK &&
            NewObjectIdentity(Domain::CompNode, &node) == S_OK && NewObjectIdentity(Domain::CompositionVisual, &visual) == S_OK &&
            NewObjectIdentity(Domain::Resource, &resource) == S_OK, "enabled epoch-qualified identities across domains");
        Check(core.Id == 1 && peer.Id == 2 && node.Id == 3 && visual.Id == 4 && resource.Id == 5, "no address primary identity");
        ObjectLifetime lifetime;
        lifetime.Object = core;
        lifetime.TypeId = {true, 42};
        lifetime.TypeClassification = {true, {2, 2, "runtime bridge"}};
        Check(Trace<Event::ObjectLifetime>([&]() noexcept { return lifetime; }) == S_OK, "native lifetime schema");
        TypeResolved type{42, "Button", lifetime.TypeClassification};
        Check(Trace<Event::TypeResolved>([&]() noexcept { return type; }) == S_OK, "native type schema");
        CorePeerAssociation association{core, peer, true, {}};
        Check(Trace<Event::CorePeerAssociation>([&]() noexcept { return association; }) == S_OK, "native core/peer schema");
        TreeEdge edge;
        edge.Child = core;
        edge.RootId = info.Process.InstanceId;
        edge.DispatcherId = 7;
        edge.Connected = true;
        edge.Membership = 2;
        Check(Trace<Event::TreeEdge>([&]() noexcept { return edge; }) == S_OK, "native scoped tree schema");
        Value pixels;
        pixels.Kind = 1; pixels.Unit = 4; pixels.Number = 3.5; pixels.DataQuality = {};
        VisualDelta delta{visual, 0, pixels, {}};
        Check(Trace<Event::VisualDelta>([&]() noexcept { return delta; }) == S_OK, "native visual schema");
        Correlation correlation{core, node, 1, true, {}};
        Check(Trace<Event::Correlation>([&]() noexcept { return correlation; }) == S_OK, "native many-domain correlation schema");
        Boundary boundary;
        boundary.OperationId = 7;
        Check(Trace<Event::Boundary>([&]() noexcept { return boundary; }) == S_OK, "native boundary schema");
        Device device{info.Process.InstanceId, 0, "D3D11", {}};
        Check(Trace<Event::Device>([&]() noexcept { return device; }) == S_OK, "native device schema");
        Resource nativeResource;
        nativeResource.Object = resource;
        nativeResource.DeviceEpoch = device.DeviceEpoch;
        nativeResource.WidthPixels = {true, 64};
        nativeResource.ResourceClassification = {1, 1, "D3D11 creation API"};
        Check(Trace<Event::Resource>([&]() noexcept { return nativeResource; }) == S_OK, "native resource schema");
        Allocation allocation;
        allocation.AllocationId = 1;
        allocation.RequestedBytes = 64;
        allocation.AllocatorEpoch = info.Process.InstanceId;
        Check(Trace<Event::Allocation>([&]() noexcept { return allocation; }) == S_OK, "native allocator schema without debug headers or HeapSize");
        Loss loss;
        loss.Reason = "synthetic test loss, not transport proof";
        Check(Trace<Event::Loss>([&]() noexcept { return loss; }) == S_OK, "native unlocalized loss schema");
        XamlPropertyChanged property;
        property.Object = core;
        property.PropertyId = 37;
        property.DeclaringType = "FrameworkElement";
        property.Name = "Width";
        property.Previous.Unit = 4;
        property.Current = pixels;
        property.Current.Number = 37.5;
        property.ChangeCause = {2, {true, {info.ProducerEpoch, 9, edge.RootId, 7}}, nullptr};
        Check(Trace<Event::XamlPropertyChanged>([&]() noexcept { return property; }) == S_OK, "native typed property/causal schema");
        Decoded decoded;
        session.Read(OnEvent, &decoded);
        Check(!decoded.Failed && decoded.Counts[1] > 0, "TDH decodes concrete native capability schema");
        for (uint16_t id = 2; id <= 16; ++id)
            if (id != 13 && id != 14) Check(decoded.Counts[id] == 1, "TDH decoded every implemented native schema exactly once");
        std::puts("PASS: actual typed native ETW schema");
    }

    void WINAPI OnArrayEvent(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId) return;
        auto& decoded = *static_cast<Decoded*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            Check(id == 1 || id == 13 || id == 14 || id == 15, "array schema event identity");
            ++decoded.Counts[id];
            ULONG bytes = 0;
            Check(TdhGetEventInformation(record, 0, nullptr, nullptr, &bytes) == ERROR_INSUFFICIENT_BUFFER, "array metadata size");
            std::vector<BYTE> metadata(bytes);
            auto info = reinterpret_cast<TRACE_EVENT_INFO*>(metadata.data());
            Check(TdhGetEventInformation(record, 0, nullptr, info, &bytes) == ERROR_SUCCESS, "actual typed array TDH metadata");
            Check(Field<uint8_t>(record, {L"EventSchemaVersion"}) == 1, "complete arrays remain fixed schema one");
            if (id == 1)
            {
                WireType(info, metadata.data(), {L"Capabilities", L"Version"}, TDH_INTYPE_INT32);
                const auto count = Field<uint32_t>(record, {L"CapabilityCount"});
                Check(count == 0 || count == 2, "honest empty automatic advertisement or explicit unavailable schema fixture");
                if (count)
                {
                    Check(Field<uint8_t>(record, {L"Capabilities", L"Capability"}, 1) == 11, "nonempty capability array second element");
                    Check(Field<uint8_t>(record, {L"Capabilities", L"Quality", L"Kind"}, 1) == 2, "schema fixture does not advertise actual supported observations");
                }
            }
            else if (id == 15)
            {
                Check(Field<uint8_t>(record, {L"HasDomains"}) == 1, "localized loss has explicit presence");
                WireType(info, metadata.data(), {L"Domains", L"EventId"}, TDH_INTYPE_UINT16);
                Check(Field<uint16_t>(record, {L"Domains", L"EventId"}, 1) == 15, "localized second delivery domain");
                Check(Field<uint8_t>(record, {L"Domains", L"EventVersion"}, 1) == 1, "localized domain preserves catalog version");
            }
            else if (id == 13)
            {
                Check(record->EventHeader.EventDescriptor.Keyword == 128, "capture keyword");
                WireType(info, metadata.data(), {L"Domains", L"SettledCut", L"Value", L"SettledThrough"}, TDH_INTYPE_UINT64);
                Check(Field<uint16_t>(record, {L"Domains", L"Domain", L"EventId"}, 1) == 15, "capture domain vector second element");
                Check(Field<uint8_t>(record, {L"Domains", L"SettledCut", L"Present"}, 1) == 1, "capture domain has typed optional cut");
                Check(Field<uint32_t>(record, {L"ChunkCount", L"Value"}) == 1, "field after variable vector decodes");
            }
            else
            {
                Check(record->EventHeader.EventDescriptor.Keyword == 128, "chunk keyword");
                Check(info->PropertyCount == 127, "full capture schema includes array counts within TDH's 128-property limit");
                WireType(info, metadata.data(), {L"Values", L"Number"}, TDH_INTYPE_DOUBLE);
                WireType(info, metadata.data(), {L"Objects", L"TypeName"}, TDH_INTYPE_ANSISTRING, TDH_OUTTYPE_UTF8);
                Check(Field<uint32_t>(record, {L"Objects", L"KeyIndex"}, 1) == 2 &&
                    Field<uint64_t>(record, {L"Keys", L"RuntimeObjectId"}, 1) == 2, "typed object table preserves second variable-width element");
                Check(Field<uint32_t>(record, {L"Objects", L"SizeIndex"}, 1) == 2 &&
                    Field<int64_t>(record, {L"Values", L"Integer"}, 1) == 80, "typed referenced object byte size");
                const auto missingQuality = Field<uint32_t>(record, {L"Values", L"QualityIndex"}, 0);
                Check(missingQuality != 0 && Field<uint8_t>(record, {L"Qualities", L"Kind"}, missingQuality - 1) == 2 &&
                    Field<uint8_t>(record, {L"Qualities", L"Coverage"}, missingQuality - 1) == 0,
                    "typed capture preserves Missing measurement with independent Complete coverage");
                const auto xamlClass = Field<uint32_t>(record, {L"Objects", L"ClassificationIndex"}, 0);
                Check(xamlClass != 0 && Field<uint8_t>(record, {L"Classifications", L"Kind"}, xamlClass - 1) == 2 &&
                    Field<uint8_t>(record, {L"Classifications", L"Evidence"}, xamlClass - 1) == 2 &&
                    Field<uint8_t>(record, {L"Objects", L"HasAddress"}, 0) == 1 &&
                    Field<uint64_t>(record, {L"Objects", L"Address"}, 0) == 0x1234,
                    "typed capture retains optional address and referenced XAML classification evidence");
                Check(Field<uint8_t>(record, {L"Ownership", L"Evidence"}, 0) == 1, "typed ownership evidence array");
                Check(Field<uint64_t>(record, {L"DispatcherId"}) == 7 &&
                    Field<uint32_t>(record, {L"Tree", L"ChildIndex"}, 0) != 0, "typed topology inherits validated chunk scope");
                const auto node = Field<uint32_t>(record, {L"Correlations", L"ToIndex"}, 0);
                Check(Field<uint8_t>(record, {L"Keys", L"Domain"}, node - 1) == 2, "typed referenced correlation");
                const auto resourceClass = Field<uint32_t>(record, {L"Resources", L"ClassificationIndex"}, 0);
                Check(resourceClass != 0 && Field<uint8_t>(record, {L"Classifications", L"Kind"}, resourceClass - 1) == 1 &&
                    Field<uint8_t>(record, {L"Classifications", L"Evidence"}, resourceClass - 1) == 1 &&
                    Field<uint32_t>(record, {L"Resources", L"WidthPixels"}, 0) == 64 &&
                    Field<uint32_t>(record, {L"Resources", L"HeightPixels"}, 0) == 32 &&
                    Field<uint8_t>(record, {L"Resources", L"HasAllocationBytes"}, 0) == 1 &&
                    Field<uint64_t>(record, {L"Resources", L"AllocationBytes"}, 0) == 4096,
                    "typed resource array retains sizes, optional backing and contextual classification evidence");
                const auto pixels = Field<uint32_t>(record, {L"Visuals", L"ValueIndex"}, 0);
                Check(Field<double>(record, {L"Values", L"Number"}, pixels - 1) == 3.5, "typed referenced visual value");
                const auto visualCause = Field<uint32_t>(record, {L"Visuals", L"CauseIndex"}, 0);
                Check(visualCause != 0 && Field<uint8_t>(record, {L"Causes", L"Kind"}, visualCause - 1) == 2 &&
                    Field<uint8_t>(record, {L"Causes", L"HasAttempt"}, visualCause - 1) == 1 &&
                    Field<uint64_t>(record, {L"Causes", L"AttemptId"}, visualCause - 1) == 9 &&
                    Field<uint64_t>(record, {L"Causes", L"DispatcherId"}, visualCause - 1) == 7,
                    "typed shared cause retains explicit epoch-qualified attempt semantics");
                const auto peer = Field<uint32_t>(record, {L"Associations", L"PeerIndex"}, 0);
                Check(Field<uint8_t>(record, {L"Keys", L"Domain"}, peer - 1) == 1, "typed referenced association");
                Check(Field<uint64_t>(record, {L"Types", L"TypeId"}, 0) == 42, "typed type array");
                Check(Field<uint64_t>(record, {L"Allocations", L"RequestedBytes"}, 0) == 64, "typed allocation array after all preceding arrays");
            }
        }
        catch (...) { decoded.Failed = true; }
    }

    void ArraySchemaTest()
    {
        Session session;
        session.Enable(129);
        ProtocolInfo info;
        Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK, "array schema handshake");
        CapabilityEntry capabilities[] = {{0, 1, {2, 2, "synthetic schema fixture, unavailable"}},
            {11, 2, {2, 1, "synthetic schema fixture, unavailable"}}};
        RuntimeCapabilities advertisement{{capabilities, 2}};
        Check(Trace<Event::RuntimeCapabilities>([&]() noexcept { return advertisement; }) == S_OK,
            "schema-one nonempty typed capability array writer");
        DomainReference domains[] = {{9, 1}, {15, 1}};
        Loss loss;
        loss.Reason = "synthetic localized loss";
        loss.HasDomains = true;
        loss.Domains = {domains, 2};
        Check(Trace<Event::Loss>([&]() noexcept { return loss; }) == S_OK, "schema-one localized loss array writer");
        DomainRange ranges[2];
        for (uint32_t i = 0; i < 2; ++i)
        {
            SequenceCut cut;
            Check(GetSequenceCut(1, domains[i].EventId, sizeof(cut), &cut) == S_OK, "actual cut for schema vector");
            ranges[i].Domain = domains[i];
            ranges[i].BeforeSequence = {true, 0};
            ranges[i].ThroughSequence = {true, cut.SettledThrough};
            ranges[i].Cut = {true, {cut.CutId, cut.ReservedThrough, cut.SettledThrough}};
        }
        CaptureBoundary boundary;
        boundary.CaptureId = info.Process.InstanceId;
        boundary.Phase = 1;
        boundary.Domains = {ranges, 2};
        boundary.ChunkCount = {true, 1};
        Check(Trace<Event::CaptureBoundary>([&]() noexcept { return boundary; }) == S_OK, "schema-one typed capture boundary vector writer");
        ObjectIdentity core{Domain::XamlCore, info.ProducerEpoch, 1}, peer{Domain::XamlPeer, info.ProducerEpoch, 2},
            node{Domain::CompNode, info.ProducerEpoch, 3}, visual{Domain::CompositionVisual, info.ProducerEpoch, 4},
            resource{Domain::Resource, info.ProducerEpoch, 5};
        SnapshotObject objects[2];
        objects[0].Key = core;
        objects[0].TypeName = "first";
        objects[0].Address = {true, 0x1234};
        objects[0].TypeClassification = {true, {2, 2, "fixture runtime bridge"}};
        objects[0].Size.DataQuality = {2, 0, "missing measurement with complete observation coverage"};
        objects[1].Key = peer;
        objects[1].TypeName = "second variable-width \xc2\xb5 name";
        objects[1].Size = {0, 1, 80, 0, false, nullptr, {}};
        OwnershipEdge ownership{core, peer, "synthetic uncertain retention", 1, {0, 1, "schema fixture"}};
        TreeEdge tree;
        tree.Child = core; tree.RootId = info.Process.InstanceId; tree.DispatcherId = 7;
        Correlation correlation{core, node, 1, true, {}};
        Resource nativeResource;
        nativeResource.Object = resource; nativeResource.DeviceEpoch = info.Process.InstanceId;
        nativeResource.WidthPixels = {true, 64};
        nativeResource.HeightPixels = {true, 32};
        nativeResource.BackingAllocationId = {true, info.Process.InstanceId};
        nativeResource.AllocationBytes = {true, 4096}; nativeResource.SizeProvenance = 0;
        nativeResource.Owner = {true, core}; nativeResource.ResourceClassification = {1, 1, "fixture creation API"};
        Value pixels{1, 4, 0, 3.5, false, nullptr, {}};
        VisualDelta delta{visual, 0, pixels, {}};
        delta.ChangeCause = {2, {true, {info.ProducerEpoch, 9, tree.RootId, 7}}, nullptr};
        CorePeerAssociation association{core, peer, true, {}};
        TypeResolved type{42, "Button", {}};
        Allocation allocation;
        allocation.AllocationId = 1; allocation.AllocatorEpoch = info.Process.InstanceId; allocation.RequestedBytes = 64;
        CaptureChunk chunk;
        chunk.CaptureId = boundary.CaptureId; chunk.RootId = tree.RootId; chunk.DispatcherId = 7;
        chunk.Objects = {objects, 2}; chunk.Ownership = {&ownership, 1}; chunk.Tree = {&tree, 1};
        chunk.Correlations = {&correlation, 1}; chunk.Resources = {&nativeResource, 1}; chunk.Visuals = {&delta, 1};
        chunk.Associations = {&association, 1}; chunk.Types = {&type, 1}; chunk.Allocations = {&allocation, 1};
        Check(Trace<Event::CaptureChunk>([&]() noexcept { return chunk; }) == S_OK, "schema-one full typed capture state arrays writer");
        Decoded decoded;
        session.Read(OnArrayEvent, &decoded);
        Check(!decoded.Failed && decoded.Counts[13] == 1 && decoded.Counts[14] == 1 && decoded.Counts[15] == 1,
            "TDH decodes nonempty capability/loss/boundary and every capture state array");
        Check(info.SupportedEvents == 0x1fffe, "schema foundation supports all sixteen catalog writers");
        Check(RequestCapture(1, 0, boundary.CaptureId) == E_NOTIMPL && info.CapabilityCount == 0, "schema completion does not implement capture algorithms or advertise coverage");
        std::puts("PASS: full schema-one typed arrays and capture wire conformance");
    }

    void ArrayValidationTest()
    {
        CapabilityEntry entries[] = {{0, 1, {}}, {0, 1, {}}};
        RuntimeCapabilities capabilities{{entries, 2}};
        Check(Validate(capabilities) == E_INVALIDARG, "capability array rejects duplicate capabilities");
        capabilities.Capabilities = {nullptr, 1};
        Check(Validate(capabilities) == E_INVALIDARG, "nonempty capability array requires storage");
        capabilities.Capabilities = {entries, 129};
        Check(Validate(capabilities) == E_INVALIDARG, "capability count is bounded before access");
        capabilities.Capabilities = {entries, 1};
        entries[0].Version = 0;
        Check(Validate(capabilities) == E_INVALIDARG, "capability version is positive");
        DomainReference domain{9, 2};
        Loss loss; loss.Reason = "localized"; loss.HasDomains = true; loss.Domains = {&domain, 1};
        Check(Validate(loss) == E_INVALIDARG, "localized domain rejects unsupported catalog versions");
        domain.Version = 1; domain.EventId = 17;
        Check(Validate(loss) == E_INVALIDARG, "localized domain rejects unknown definitions");
        loss.Domains = {nullptr, 1};
        Check(Validate(loss) == E_INVALIDARG, "localized loss validates nonempty storage");
        loss.Domains = {&domain, 257};
        Check(Validate(loss) == E_INVALIDARG, "localized loss count is bounded");
        loss.HasDomains = false;
        Check(Validate(loss) == S_OK, "absent loss domains ignore inactive backing storage");
        DomainRange range;
        range.Domain = {14, 1}; range.BeforeSequence = {true, 1}; range.ThroughSequence = {true, 2};
        range.Cut = {true, {RuntimeProviderId, 2, 2}};
        CaptureBoundary boundary;
        boundary.CaptureId = RuntimeProviderId; boundary.Phase = 1;
        boundary.ChunkCount = {true, 0}; boundary.Domains = {&range, 1};
        Check(Validate(boundary) == S_OK, "complete End has a typed settled vector");
        range.Cut.Value.SettledThrough = 1;
        Check(Validate(boundary) == E_INVALIDARG, "complete End cannot outrun settlement");
        range.Cut.Value.SettledThrough = 3;
        Check(Validate(boundary) == E_INVALIDARG, "settlement cannot exceed reservations");
        range.Cut.Value.SettledThrough = 2; range.BeforeSequence.Present = false;
        Check(Validate(boundary) == E_INVALIDARG, "complete vector requires its before checkpoint");
        range.BeforeSequence.Present = true; boundary.ChunkCount.Present = false;
        Check(Validate(boundary) == E_INVALIDARG, "End requires an explicit chunk count");
        boundary.ChunkCount.Present = true;
        DomainRange duplicated[] = {range, range}; boundary.Domains = {duplicated, 2};
        Check(Validate(boundary) == E_INVALIDARG, "capture vectors reject repeated delivery domains");
        CaptureChunk chunk;
        chunk.CaptureId = RuntimeProviderId; chunk.RootId = RuntimeProviderId; chunk.DispatcherId = 1;
        Check(Validate(chunk) == S_OK, "empty full-state arrays are explicit valid schema values");
#define CHECK_ARRAY_BOUND(member, type, maximum) \
        { type item{}; auto invalid = chunk; invalid.member = {&item, maximum + 1}; \
          Check(Validate(invalid) == E_INVALIDARG, #member " count bounded before access"); \
          invalid.member = {nullptr, 1}; Check(Validate(invalid) == E_INVALIDARG, #member " nonempty storage required"); }
        CHECK_ARRAY_BOUND(Objects, SnapshotObject, 4096)
        CHECK_ARRAY_BOUND(Ownership, OwnershipEdge, 8192)
        CHECK_ARRAY_BOUND(Tree, TreeEdge, 4096)
        CHECK_ARRAY_BOUND(Correlations, Correlation, 4096)
        CHECK_ARRAY_BOUND(Resources, Resource, 4096)
        CHECK_ARRAY_BOUND(Visuals, VisualDelta, 4096)
        CHECK_ARRAY_BOUND(Associations, CorePeerAssociation, 4096)
        CHECK_ARRAY_BOUND(Types, TypeResolved, 4096)
        CHECK_ARRAY_BOUND(Allocations, Allocation, 4096)
#undef CHECK_ARRAY_BOUND
        TreeEdge edge; edge.Child = {Domain::XamlCore, RuntimeProviderId, 1};
        edge.RootId = RuntimeProviderId; edge.DispatcherId = 2; chunk.Tree = {&edge, 1};
        Check(Validate(chunk) == E_INVALIDARG, "nested topology cannot escape its chunk dispatcher");
        std::puts("PASS: bounded arrays, optionals, catalog domains and capture settlement validation");
    }

    struct BudgetExpected { uint32_t Objects; uint64_t Sequence; };
    struct BudgetDecoded { bool Failed = false; uint32_t Count = 0; uint32_t LargestWire = 0; uint32_t LastData = 0; uint32_t DataStep = 0; std::vector<BudgetExpected> Expected; };
    void WINAPI OnBudgetEvent(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId) return;
        auto& state = *static_cast<BudgetDecoded*>(record->UserContext);
        try
        {
            Check(state.Count < state.Expected.size(), "only budget-accepted native writes appear");
            const auto expected = state.Expected[state.Count++];
            Check(Field<uint16_t>(record, {L"SemanticEventId"}) == 14 &&
                Field<uint64_t>(record, {L"SourceSequence"}) == expected.Sequence, "budget writes retain actual reservations");
            Check(Field<uint64_t>(record, {L"Keys", L"RuntimeObjectId"}, expected.Objects - 1) == 1, "TDH decodes large nonempty typed table");
            uint32_t wireBytes = record->UserDataLength;
            for (USHORT i = 0; i < record->ExtendedDataCount; ++i)
            {
                const auto& extension = record->ExtendedData[i];
                if (extension.ExtType == EVENT_HEADER_EXT_TYPE_EVENT_SCHEMA_TL || extension.ExtType == EVENT_HEADER_EXT_TYPE_PROV_TRAITS)
                    wireBytes += extension.DataSize;
            }
            Check(wireBytes <= MaximumWireBytes, "real payload plus typed metadata fits the wire budget");
            state.LargestWire = wireBytes;
            if (state.LastData) state.DataStep = record->UserDataLength - state.LastData;
            state.LastData = record->UserDataLength;
        }
        catch (...) { state.Failed = true; }
    }

    void WireBudgetTest()
    {
        Session session; session.Enable(128);
        ProtocolInfo info; Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK, "capture-only epoch handshake");
        ObjectIdentity baseline;
        Check(NewObjectIdentity(Domain::XamlCore, &baseline) == S_OK && baseline.Epoch == info.ProducerEpoch,
            "capture-only schema uses enabled epoch-qualified object identities");
        char name[1025]; memset(name, 'x', sizeof(name) - 1); name[sizeof(name) - 1] = 0;
        std::vector<SnapshotObject> objects(48);
        for (auto& item : objects) { item.Key = {Domain::XamlCore, info.ProducerEpoch, 1}; item.TypeName = name; }
        CaptureChunk chunk;
        chunk.CaptureId = info.Process.InstanceId; chunk.RootId = info.Process.InstanceId; chunk.DispatcherId = 1;
        chunk.Objects = {objects.data(), 1};
        objects[0].Key.Epoch = RuntimeProviderId;
        Check(Trace<Event::CaptureChunk>([&]() noexcept { return chunk; }) == E_INVALIDARG, "nested identities reject a foreign producer epoch");
        objects[0].Key.Epoch = info.ProducerEpoch;
        BudgetDecoded decoded;
        uint32_t rejected = 0;
        for (uint32_t count = 35; count <= 48; ++count)
        {
            chunk.Objects.Count = count;
            const HRESULT result = Trace<Event::CaptureChunk>([&]() noexcept { return chunk; });
            SequenceCut cut;
            Check(GetSequenceCut(1, 14, sizeof(cut), &cut) == S_OK && cut.ReservedThrough == cut.SettledThrough,
                "oversized and accepted native writes both settle terminal attempts");
            if (result == S_OK) decoded.Expected.push_back({count, cut.ReservedThrough});
            else { Check(result == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW), "oversized wire has an explicit bounded failure"); ++rejected; }
        }
        Check(!decoded.Expected.empty() && rejected != 0, "exercise accepted near-limit and rejected oversized native arrays");
        session.Read(OnBudgetEvent, &decoded);
        Check(!decoded.Failed && decoded.Count == decoded.Expected.size() && decoded.LargestWire > 45 * 1024,
            "actual TDH coverage of near-budget arrays without oversized writes");
        Check(decoded.LastData + decoded.DataStep <= MaximumWireBytes && decoded.LargestWire + decoded.DataStep > MaximumWireBytes,
            "metadata contributes to rejection even when the next payload alone fits");
        std::puts("PASS: actual ETW typed-array wire budget and failed-attempt settlement");
    }

    void TransitionTest()
    {
        auto frame = []() noexcept { Frame value; value.Outcome = 3; return value; };
        Session first;
        first.Enable(17);
        Check(Trace<Event::Frame>(frame) == S_OK, "initial frame");
        SequenceCut before;
        Check(GetSequenceCut(1, 9, sizeof(before), &before) == S_OK && before.ReservedThrough == 1 && before.SettledThrough == 1, "actual immutable producer cut");
        {
            Session second;
            second.Enable(3);
            Check(Trace<Event::Frame>(frame) == S_OK, "other listener does not disable frames");
            SequenceCut during;
            Check(GetSequenceCut(1, 9, sizeof(during), &during) == S_OK && during.ProducerEpoch == before.ProducerEpoch &&
                during.ReservedThrough == 2 && during.SettledThrough == 2, "another listener cannot reset continuity");
            Check(before.ReservedThrough == 1 && before.CutId != during.CutId, "cut IDs bind immutable facts");
            second.Disable();
        }
        HRESULT nested = E_UNEXPECTED;
        Check(Trace<Event::Frame>([&]() noexcept
        {
            nested = Trace<Event::Frame>(frame);
            return frame();
        }) == S_OK, "outer instrumentation survives nested work");
        Check(nested == S_FALSE, "reentrant instrumentation suppressed before payload work");
        Emission pending;
        Check(BeginEmission(Event::Frame, &pending) == S_OK, "reserve before terminal write");
        const bool recursiveEntry = EnterInstrumentation();
        if (recursiveEntry) LeaveInstrumentation();
        first.Disable();
        const HRESULT failedWrite = Write(pending, frame());
        EndEmission(&pending);
        Check(!recursiveEntry, "manual emission owns an internal-work reentrancy scope through terminal write");
        Check(FAILED(failedWrite), "disable race returns actual non-write failure, not fabricated success");
        SequenceCut terminal;
        Check(GetSequenceCut(1, 9, sizeof(terminal), &terminal) == S_OK && terminal.ReservedThrough == 4 &&
            terminal.SettledThrough == 4, "failed/skipped write settles, but claims no listener receipt");
        int expensive = 0;
        Check(Trace<Event::Frame>([&]() noexcept { ++expensive; return frame(); }) == S_FALSE && expensive == 0, "disable transition avoids helper");
        ObjectIdentity disabled;
        Check(NewObjectIdentity(Domain::XamlCore, &disabled) == S_FALSE && disabled.Id == 0, "disabled epoch has no object allocation/mapping");
        first.Enable(17);
        Check(Trace<Event::Frame>(frame) == S_OK, "re-enabled write");
        SequenceCut fresh;
        Check(GetSequenceCut(1, 9, sizeof(fresh), &fresh) == S_OK && fresh.ProducerEpoch != before.ProducerEpoch &&
            fresh.ReservedThrough == 1 && fresh.SettledThrough == 1, "all-listener disable/re-enable starts a fresh baseline epoch");
        Check(GetSequenceCut(2, 9, sizeof(fresh), &fresh) == HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "cut rejects incompatible version");
        Check(GetSequenceCut(1, 13, sizeof(fresh), &fresh) == S_OK && !IsEnabled(Event::CaptureChunk),
            "capture has a schema domain but is not enabled or implemented as an algorithm");
        std::puts("PASS: actual multi-listener enable transitions, immutable cuts, reentrancy, failed writes");
    }

    void HandshakeSchemaTest()
    {
        Session session;
        session.Enable(1);
        ProtocolInfo info;
        Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK, "diagnostics handshake with capabilities listener");
        Decoded decoded;
        session.Read(OnEvent, &decoded);
        Check(!decoded.Failed && decoded.Counts[1] == 1, "handshake emits actual native capabilities without later hooks");
        std::puts("PASS: diagnostics handshake native capabilities");
    }

    void LevelFilterTest()
    {
        Session capabilities;
        capabilities.Enable(1);
        Session lowLevelFrames;
        lowLevelFrames.Enable(16, 3);
        int expensive = 0;
        Check(Trace<Event::Frame>([&]() noexcept { ++expensive; Frame value; value.Outcome = 3; return value; }) == S_FALSE &&
            expensive == 0, "separate listeners cannot combine level/keyword eligibility");
        lowLevelFrames.Enable(16, 4, 8);
        Check(Trace<Event::Frame>([&]() noexcept { ++expensive; Frame value; value.Outcome = 3; return value; }) == S_FALSE &&
            expensive == 0, "MatchAll remains bound to the same listener's event definition");
        lowLevelFrames.Enable(16, 0);
        Check(Trace<Event::Frame>([&]() noexcept { ++expensive; Frame value; value.Outcome = 3; return value; }) == S_OK &&
            expensive == 1, "zero ETW level enables all levels without resetting another listener's epoch");
        std::puts("PASS: native per-definition level eligibility");
    }

    void InvalidPayloadTest()
    {
        alignas(8) uint8_t canary[8];
        memset(canary, 0x5a, sizeof(canary));
        Check(GetProtocolInfo(1, sizeof(canary), reinterpret_cast<ProtocolInfo*>(canary)) == E_INVALIDARG &&
            canary[0] == 0x5a && canary[7] == 0x5a, "undersized ABI buffers are not overwritten");
        Frame frame;
        frame.Outcome = 1;
        Check(Validate(frame) == E_INVALIDARG, "submission cannot omit its qualified attempt/ID/result");
        frame.Outcome = 3;
        Check(Validate(frame) == S_OK, "standalone needs no invented attempt");
        TypeResolved type;
        type.TypeId = 1;
        type.Name = "\xc0\xaf";
        Check(Validate(type) == E_INVALIDARG, "reject invalid UTF-8 before ETW string helpers");
        char large[1026];
        memset(large, 'x', sizeof(large) - 1);
        large[sizeof(large) - 1] = 0;
        type.Name = large;
        Check(Validate(type) == E_INVALIDARG, "bounded native strings distinct from normalized payload budget");
        VisualDelta visual;
        visual.Visual = {Domain::CompositionVisual, RuntimeProviderId, 1};
        visual.PropertyValue.Kind = 1;
        visual.PropertyValue.Unit = 4;
        visual.PropertyValue.DataQuality = {};
        visual.PropertyValue.Number = std::numeric_limits<double>::infinity();
        Check(Validate(visual) == E_INVALIDARG, "reject non-finite native numeric observations");
        Resource resource;
        resource.Object = {Domain::Resource, RuntimeProviderId, 1};
        resource.DeviceEpoch = RuntimeProviderId;
        resource.SizeProvenance = 0;
        Check(Validate(resource) == E_INVALIDARG, "actual allocation cannot be fabricated from dimensions/unknown backing");
        XamlPropertyChanged property;
        property.Object = {Domain::XamlCore, RuntimeProviderId, 1};
        property.PropertyId = 1;
        property.DeclaringType = "Element";
        property.Name = "Width";
        property.Previous.Unit = 4;
        property.Current.Unit = 1;
        Check(Validate(property) == E_INVALIDARG, "property units do not silently change");
        visual.Property = 2;
        visual.PropertyValue.Number = -1;
        Check(Validate(visual) == E_INVALIDARG, "negative visual dimensions violate the shared observation contract");
        ObjectLifetime lifetime;
        lifetime.Object = {Domain::XamlCore, RuntimeProviderId, 1};
        lifetime.TypeClassification = {true, {3, 1, "runtime type"}};
        Check(Validate(lifetime) == E_INVALIDARG, "core classification cannot claim not-applicable");
        lifetime.Object.IdentityDomain = Domain::CompositionVisual;
        lifetime.TypeClassification.Value.Kind = 2;
        Check(Validate(lifetime) == E_INVALIDARG, "composition identity cannot claim authoritative UIElement classification");
        frame.Reference = {true, {RuntimeProviderId, 1, RuntimeProviderId, 1}};
        Check(Validate(frame) == S_OK, "standalone commits may retain a known qualified attempt without a submitted frame");
        frame.Outcome = 0;
        frame.HResult = {true, S_OK};
        Check(Validate(frame) == E_INVALIDARG, "attempt begins cannot claim a terminal result");
        frame.Outcome = 4;
        frame.HResult.Present = false;
        Check(Validate(frame) == E_INVALIDARG, "attempt completion requires an explicit successful result");
        frame.HResult = {true, E_FAIL};
        Check(Validate(frame) == E_INVALIDARG, "failed attempts cannot masquerade as successful completion");
        frame.HResult.Value = S_OK;
        Check(Validate(frame) == S_OK, "successful attempt completion remains a separate typed outcome");
        Allocation freed;
        freed.AllocationId = 1;
        freed.Operation = 1;
        freed.RequestedBytes = 8;
        freed.ActualBytes = {true, 0};
        freed.AllocatorEpoch = RuntimeProviderId;
        Check(Validate(freed) == S_OK, "free observations do not claim an allocation-size lower bound");
        type.Name = " \t\r\n";
        Check(Validate(type) == E_INVALIDARG, "required native text rejects ASCII whitespace only");
        type.Name = "\xc2\xa0\xe2\x80\x83\xe3\x80\x80";
        Check(Validate(type) == E_INVALIDARG, "required native text rejects Unicode whitespace only");
        type.Name = "\xe2\x80\x83\xe4\xb8\xad";
        Check(Validate(type) == S_OK, "required native text accepts non-whitespace Unicode");
        std::puts("PASS: ABI canaries, malformed native input and strict typed observations");
    }
}

#ifdef WINUI_PROFILER_STANDALONE
int main()
{
    try
    {
        ProtocolTest();
        SequenceTest();
        QualityCrossProductTest();
        IndependentEpochTest();
        EnabledTest();
        SchemaTest();
        ArraySchemaTest();
        ArrayValidationTest();
        WireBudgetTest();
        TransitionTest();
        LevelFilterTest();
        HandshakeSchemaTest();
        InvalidPayloadTest();
        return 0;
    }
    catch (const std::exception&) { return 1; }
}
#else
void ProfilerInstrumentationTests::ProtocolAndSequence()
{
    ProtocolTest();
    SequenceTest();
    QualityCrossProductTest();
    IndependentEpochTest();
    EnabledTest();
    SchemaTest();
    ArraySchemaTest();
    ArrayValidationTest();
    WireBudgetTest();
    TransitionTest();
    LevelFilterTest();
    HandshakeSchemaTest();
    InvalidPayloadTest();
}
#endif
