// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "ProfilerInstrumentationTests.h"
#include "WinUIProfilerRuntimeBridge.h"
#include "WinUIProfilerTracing.h"
#include "WinUIProfilerHooks.h"
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
#include <new>
#include <string>
#include <weakreference.h>

using namespace WinUIProfiler;

namespace
{
#ifdef WINUI_PROFILER_STANDALONE
    const wchar_t* fixtureDirectory = nullptr;
#endif
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
#ifdef WINUI_PROFILER_STANDALONE
            if (fixtureDirectory)
                Check(swprintf_s(m_properties.File, L"%ls\\%ls.etl", fixtureDirectory, m_name) > 0, "bounded fixture ETL path");
#endif
            const ULONG result = StartTraceW(&m_handle, m_name, &m_properties.Properties);
            if (result) std::printf("StartTrace(%ls) failed: %lu\n", m_name, result);
            Check(result == ERROR_SUCCESS, "start isolated ETW session");
        }
        ~Session()
        {
            if (m_handle) ControlTraceW(m_handle, m_name, &m_properties.Properties, EVENT_TRACE_CONTROL_STOP);
#ifdef WINUI_PROFILER_STANDALONE
            if (!fixtureDirectory)
#endif
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
#ifdef WINUI_PROFILER_STANDALONE
            if (fixtureDirectory) std::printf("FIXTURE: %ls\n", m_properties.File);
            else
#endif
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

    class NonWeakVisual final : public IUnknown
    {
    public:
        ULONG References = 1;
        unsigned WeakQueries = 0;
        unsigned CanonicalQueries = 0;
        HRESULT CanonicalResult = S_OK;
        HRESULT WeakResult = E_NOINTERFACE;
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
        {
            if (!value) return E_POINTER;
            *value = nullptr;
            if (iid == __uuidof(IUnknown))
            {
                ++CanonicalQueries;
                if (FAILED(CanonicalResult)) return CanonicalResult;
                *value = static_cast<IUnknown*>(this);
                AddRef();
                return S_OK;
            }
            ++WeakQueries;
            return WeakResult;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++References; }
        ULONG STDMETHODCALLTYPE Release() override { return --References; }
    };
    struct VisualIsolationDecoded
    {
        bool Failed = false;
        unsigned CoreLinks = 0;
        unsigned VisualLinks = 0;
        unsigned Diagnostics = 0;
    };
    void WINAPI OnVisualIsolation(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId ||
            record->EventHeader.ProcessId != GetCurrentProcessId()) return;
        auto& state = *static_cast<VisualIsolationDecoded*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            if (id == 7)
            {
                const auto kind = Field<uint8_t>(record, {L"Kind"});
                if (kind == 1) ++state.CoreLinks;
                if (kind == 2) ++state.VisualLinks;
            }
            if (id == 15)
            {
                PROPERTY_DATA_DESCRIPTOR reason{};
                reason.PropertyName = reinterpret_cast<ULONGLONG>(L"Reason");
                reason.ArrayIndex = ULONG_MAX;
                ULONG size = 0;
                Check(TdhGetPropertySize(record, 0, nullptr, 1, &reason, &size) == ERROR_SUCCESS,
                    "unsupported visual diagnostic reason exists");
                std::vector<BYTE> text(size + 1);
                Check(TdhGetProperty(record, 0, nullptr, 1, &reason, size, text.data()) == ERROR_SUCCESS,
                    "unsupported visual diagnostic reason decodes");
                Check(std::strstr(reinterpret_cast<char*>(text.data()), "0x80004002") != nullptr,
                    "unsupported visual diagnostic preserves original HRESULT");
                Check(Field<uint8_t>(record, {L"HasDomains"}) == 1, "unsupported visual diagnostic is domain scoped");
                ++state.Diagnostics;
            }
        }
        catch (...) { state.Failed = true; }
    }
    void UnsupportedVisualIsolationTest()
    {
        NonWeakVisual visual;
        int core = 0, node = 0, otherCore = 0, otherNode = 0;
        Session session;
        session.Enable(1 | 2 | 8);
        Hooks::LinkVisual(&node, &visual);
        Check(visual.WeakQueries == 1 && visual.References == 1,
            "unsupported visual is queried without retaining canonical IUnknown");
        ProtocolInfo info{};
        Check(GetProtocolInfo(1, sizeof(info), &info) == S_OK,
            "unsupported weak visual must not poison subsequent protocol info");
        Hooks::Link(&core, Domain::XamlCore, &node, Domain::CompNode, 1, true);
        Hooks::Link(&otherCore, Domain::XamlCore, &node, Domain::CompNode, 1, true);
        Hooks::Link(&core, Domain::XamlCore, &otherNode, Domain::CompNode, 1, true);
        Hooks::Link(&otherCore, Domain::XamlCore, &otherNode, Domain::CompNode, 1, true);
        Hooks::Link(&core, Domain::XamlCore, &node, Domain::CompNode, 1, false);
        VisualIsolationDecoded decoded;
        session.Read(OnVisualIsolation, &decoded);
        Check(!decoded.Failed && decoded.CoreLinks == 5,
            "unsupported visual does not suppress independent many-to-many core/comp links");
        Check(decoded.VisualLinks == 0 && decoded.Diagnostics == 1,
            "unsupported visual has explicit partial diagnostic, never fabricated visual identity/link");
        Session failure;
        failure.Enable(1 | 2 | 8);
        NonWeakVisual broken;
        broken.WeakResult = E_OUTOFMEMORY;
        Hooks::LinkVisual(&node, &broken);
        Check(GetProtocolInfo(1, sizeof(info), &info) == E_OUTOFMEMORY,
            "unexpected weak-source failure remains an explicit fatal protocol error");
        std::puts("PASS: unsupported weak visual isolation and original HRESULT diagnostic; synthetic native regression, not actual hooks");
    }

    class VisualAlias final : public IUnknown
    {
    public:
        explicit VisualAlias(NonWeakVisual& target) : m_target(target) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override { return m_target.QueryInterface(iid, value); }
        ULONG STDMETHODCALLTYPE AddRef() override { return m_target.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return m_target.Release(); }
    private:
        NonWeakVisual& m_target;
    };
    struct SlotDecoded
    {
        bool Failed = false;
        unsigned Baselines = 0, Links = 0, Unlinks = 0, Gaps = 0;
        std::vector<uint64_t> Properties;
        std::vector<GUID> Epochs;
        std::vector<uint64_t> Resources;
    };
    void WINAPI OnSlot(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId ||
            record->EventHeader.ProcessId != GetCurrentProcessId()) return;
        auto& state = *static_cast<SlotDecoded*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            if (id == 2 && Field<uint8_t>(record, {L"Object", L"Domain"}) == static_cast<uint8_t>(Domain::Resource))
                state.Resources.push_back(Field<uint64_t>(record, {L"Object", L"RuntimeObjectId"}));
            if (id == 2 && Field<uint8_t>(record, {L"Object", L"Domain"}) == 3)
            {
                Check(Field<uint8_t>(record, {L"Operation"}) == 2, "owner interval never fabricates visual birth/destruction");
                Check(Field<uint8_t>(record, {L"Quality", L"Kind"}) == 0, "owner presence is measured, not guessed");
                Check(Field<uint8_t>(record, {L"Quality", L"Coverage"}) == 1, "owner interval is explicitly partial");
                ++state.Baselines;
            }
            if (id == 7 && Field<uint8_t>(record, {L"Kind"}) == 2)
            {
                if (Field<uint8_t>(record, {L"Linked"})) ++state.Links;
                else ++state.Unlinks;
            }
            if (id == 6)
            {
                state.Properties.push_back(Field<uint64_t>(record, {L"Visual", L"RuntimeObjectId"}));
                state.Epochs.push_back(Field<GUID>(record, {L"Visual", L"ProducerEpoch"}));
            }
            if (id == 15) ++state.Gaps;
            Check(id != 5, "owner retirement does not fake physical visual disconnect");
        }
        catch (...) { state.Failed = true; }
    }
    void RuntimeLifetimeTest()
    {
        Session session;
        session.Enable(1 | 2 | 8);
        NonWeakVisual visual;
        int owner = 0;
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualProperty(&visual, 0, 1);
        Hooks::ShutdownRuntime();
        Hooks::ShutdownRuntime();
        const auto queries = visual.CanonicalQueries;
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Check(visual.CanonicalQueries == queries, "shutdown rejects observation before canonical QI");
        Hooks::InitializeRuntime();
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualProperty(&visual, 0, 2);
        Hooks::InitializeRuntime();
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualProperty(&visual, 0, 3);
        Hooks::ShutdownRuntime();
        Hooks::InitializeRuntime();
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualProperty(&visual, 0, 4);
        SlotDecoded decoded;
        session.Read(OnSlot, &decoded);
        Check(!decoded.Failed && decoded.Baselines == 3 && decoded.Links == 3 && decoded.Unlinks == 0,
            "deterministic runtime cleanup retires observations without fake native destruction/unlinks");
        Check(decoded.Properties.size() == 4 && decoded.Properties[0] != decoded.Properties[1] &&
            decoded.Properties[1] == decoded.Properties[2] && decoded.Properties[2] != decoded.Properties[3],
            "reinit cannot reuse qualified IDs; repeated initialization preserves live registry");
        Check(decoded.Epochs.size() == 4 && decoded.Epochs[0] == decoded.Epochs[3],
            "continuous listener retains producer epoch across registry shutdown/reinit");
        Check(visual.References == 1 && visual.WeakQueries == 0, "runtime lifetime has no diagnostic strong ownership");
        std::puts("PASS: actual ETW lazy registry teardown/reinit and continuous epoch-qualified IDs");
    }
#ifdef WINUI_PROFILER_STANDALONE
    class CleanupWeak final : public IWeakReference
    {
    public:
        ULONG References = 1;
        unsigned Released = 0;
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            *out = nullptr;
            if (id != __uuidof(IUnknown) && id != __uuidof(IWeakReference)) return E_NOINTERFACE;
            *out = this; AddRef(); return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++References; }
        ULONG STDMETHODCALLTYPE Release() override
        {
            if (--References == 1) { ++Released; Hooks::Born(Domain::XamlCore, this); }
            return References;
        }
        HRESULT STDMETHODCALLTYPE Resolve(REFIID, IInspectable** out) override { *out = nullptr; return S_OK; }
    };
    class CleanupVisual final : public IWeakReferenceSource
    {
    public:
        CleanupWeak Weak;
        ULONG References = 1;
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** out) override
        {
            *out = nullptr;
            if (id != __uuidof(IUnknown) && id != __uuidof(IWeakReferenceSource)) return E_NOINTERFACE;
            *out = this; AddRef(); return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return ++References; }
        ULONG STDMETHODCALLTYPE Release() override { return --References; }
        HRESULT STDMETHODCALLTYPE GetWeakReference(IWeakReference** out) override { *out = &Weak; Weak.AddRef(); return S_OK; }
    };
    void OrderlyWeakCleanupTest()
    {
        Session session;
        session.Enable(1 | 2 | 8);
        CleanupVisual visual;
        int node = 0;
        Hooks::LinkVisual(&node, &visual);
        Check(visual.Weak.References == 2, "registry owns one genuine weak-reference implementation");
        Hooks::ShutdownRuntime();
        Check(visual.Weak.References == 1 && visual.Weak.Released == 1 && visual.References == 1,
            "orderly shutdown releases weak ownership exactly once with reentrant hooks suppressed");
        Hooks::InitializeRuntime();
        std::puts("PASS: orderly genuine weak-reference release and recursion suppression");
    }
    struct LayoutGapDecoded
    {
        bool Failed = false;
        int64_t UnmatchedStart = 0, UnmatchedEnd = 0, GapStart = 0, GapEnd = 0;
        unsigned Boundaries = 0, Frames = 0, Extra = 0;
    };
    void WINAPI OnLayoutGap(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId || record->EventHeader.ProcessId != GetCurrentProcessId()) return;
        auto& state = *static_cast<LayoutGapDecoded*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            if (id != 8 && id != 9) return;
            if (id == 8) ++state.Boundaries; else ++state.Frames;
            const auto time = record->EventHeader.TimeStamp.QuadPart;
            if ((time >= state.UnmatchedStart && time <= state.UnmatchedEnd) ||
                (time >= state.GapStart && time <= state.GapEnd)) ++state.Extra;
        }
        catch (...) { state.Failed = true; }
    }
    void LayoutStackLifetimeTest()
    {
        Session continuity;
        continuity.Enable(32);
        Session frames;
        frames.Enable(16);
        int root = 0, dispatcher = 0, element = 0;
        LayoutGapDecoded observed;
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now); observed.UnmatchedStart = now.QuadPart;
        for (unsigned i = 0; i != 128; ++i)
        {
            std::thread thread([&] { Hooks::Layout(&root, &dispatcher, &element, 0, false); });
            thread.join();
            Check(Hooks::LayoutStackCountForTest() == 0, "unmatched end leaves no empty thread entry");
        }
        QueryPerformanceCounter(&now); observed.UnmatchedEnd = now.QuadPart;
        Hooks::Layout(&root, &dispatcher, &element, 0, true);
        Check(Hooks::LayoutStackCountForTest() == 1, "observed begin owns one layout stack");
        frames.Disable();
        frames.Enable(16);
        QueryPerformanceCounter(&now); observed.GapStart = now.QuadPart;
        Hooks::Layout(&root, &dispatcher, &element, 0, false);
        QueryPerformanceCounter(&now); observed.GapEnd = now.QuadPart;
        Check(Hooks::LayoutStackCountForTest() == 0, "generation-invalidated end erases its empty stack");
        frames.Read(OnLayoutGap, &observed);
        std::printf("LAYOUT_COUNTS: boundaries=%u frames=%u extra=%u\n", observed.Boundaries, observed.Frames, observed.Extra);
        Check(!observed.Failed && observed.Boundaries == 1 && observed.Frames == 1 && observed.Extra == 0,
            "actual ETW has exactly one genuine begin/frame and zero unmatched/gap-end boundary/frame records");
        std::puts("PASS: unmatched/generation-gap ends and128 actual thread lifetimes");
    }
    void TerminationChild(const char* mapping)
    {
        Check(SetEnvironmentVariableA("WINUI_PROFILER_TERMINATION_STATE", mapping), "child-only mapping environment");
        wchar_t name[100]{}, path[MAX_PATH]{};
        swprintf_s(name, L"Local\\WinUIProfiler-Termination-%lu", GetCurrentProcessId());
        HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, name);
        Check(ready != nullptr, "create owned child readiness event");
        Check(GetModuleFileNameW(nullptr, path, ARRAYSIZE(path)) != 0, "child executable identity");
        auto filename = wcsrchr(path, L'\\');
        Check(filename != nullptr, "child fixture directory");
        wcscpy_s(filename + 1, ARRAYSIZE(path) - (filename + 1 - path), L"ProfilerTerminationFixture.dll");
        Check(LoadLibraryW(path) != nullptr, "load actual owned termination fixture DLL");
        Check(WaitForSingleObject(ready, 5000) == WAIT_OBJECT_0, "actual DLL registry owner acquired lock");
        ExitProcess(0);
    }
    void TerminationTest()
    {
        Session listener;
        listener.Enable(1 | 2);
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
        wchar_t mappingName[100]{};
        swprintf_s(mappingName, L"Local\\WinUIProfiler-Detach-%lu", GetCurrentProcessId());
        HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(LONG), mappingName);
        auto state = static_cast<LONG*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(LONG)));
        Check(state != nullptr, "owned child detach phase mapping");
        std::wstring command = L"\"" + std::wstring(path) + L"\" --termination-child " + mappingName;
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION child{};
        Check(CreateProcessW(path, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &child),
            "start exact owned termination child");
        const DWORD wait = WaitForSingleObject(child.hProcess, 8000);
        DWORD code = 1;
        if (wait == WAIT_TIMEOUT) TerminateProcess(child.hProcess, 100);
        WaitForSingleObject(child.hProcess, 3000);
        GetExitCodeProcess(child.hProcess, &code);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        const LONG phase = *state;
        UnmapViewOfFile(state);
        CloseHandle(mapping);
        std::printf("termination wait=%lu exit=%lu detachPhase=%ld\n", wait, code, phase);
        Check(wait == WAIT_OBJECT_0 && code == 0, "ExitProcess cannot wait on terminated registry owner");
        Check(phase == 2, "actual DLL detach entered and completed profiler shutdown");
        std::puts("PASS: actual DLL detach while another terminated thread owns registry lock");
    }
#endif
    void VisualSlotTest()
    {
        NonWeakVisual a, b;
        VisualAlias alias(a);
        int first = 0, second = 0;
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Check(a.References == 1 && a.WeakQueries == 0 && a.CanonicalQueries == 0,
            "disabled slots perform no QI/retention");
        Session session;
        session.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &alias);
        Hooks::VisualSlot(&first, Domain::CompNode, 1, &alias);
        Hooks::VisualSlot(&second, Domain::CompNode, 0, &a);
        Hooks::VisualProperty(&a, 0, 43);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &b);
        Hooks::VisualSlot(&first, Domain::CompNode, 1, nullptr);
        Hooks::VisualProperty(&a, 0, 44);
        Hooks::ReleaseVisualOwner(&first, Domain::CompNode);
        Hooks::ReleaseVisualOwner(&second, Domain::CompNode);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Hooks::VisualProperty(&a, 0, 45);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, nullptr);
        Check(a.References == 1 && b.References == 1, "slot diagnostics retain no COM references");
        SlotDecoded decoded;
        session.Read(OnSlot, &decoded);
        Check(!decoded.Failed && decoded.Baselines == 3, "three partial visual owner intervals decoded from actual bridge ETW");
        Check(decoded.Links == 4 && decoded.Unlinks == 4, "aliases and shared slots preserve semantic many-to-many links");
        Check(decoded.Properties.size() == 3 && decoded.Properties[0] == decoded.Properties[1] &&
            decoded.Properties[1] != decoded.Properties[2], "shared owner continuity; fresh ID after last-owner gap");
        Check(decoded.Gaps == 3 && a.WeakQueries == 0 && b.WeakQueries == 0,
            "owner gaps are explicit and owned visuals never require weak QI");
        Session renewed;
        renewed.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Hooks::VisualProperty(&a, 0, 46);
        Hooks::ReleaseVisualOwner(&first, Domain::CompNode);
        SlotDecoded next;
        renewed.Read(OnSlot, &next);
        Check(!next.Failed && next.Baselines == 1 && next.Links == 1 && next.Unlinks == 1 &&
            next.Epochs.size() == 1 && next.Epochs[0] != decoded.Epochs[0],
            "reenable uses a fresh qualified partial baseline, never joins the old epoch");
        Session continuous;
        continuous.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Hooks::VisualProperty(&a, 0, 47);
        Session joining;
        joining.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &alias);
        Hooks::VisualProperty(&a, 0, 48);
        SlotDecoded joined;
        joining.Read(OnSlot, &joined);
        continuous.Enable(1 | 8);
        Hooks::VisualProperty(&a, 0, 49);
        continuous.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &a);
        Hooks::VisualProperty(&a, 0, 50);
        Hooks::ReleaseVisualOwner(&first, Domain::CompNode);
        SlotDecoded kept;
        continuous.Read(OnSlot, &kept);
        Check(!joined.Failed && !kept.Failed && joined.Properties.size() == 1 &&
            kept.Properties.size() == 4 && kept.Baselines == 2 && kept.Links == 1 &&
            kept.Unlinks == 1 && joined.Properties[0] == kept.Properties[0],
            "joining listener and lifetime-family reenable preserve continuous visual identity");
        for (const auto id : kept.Properties)
            Check(id == kept.Properties[0], "continuous visual family does not lose its qualified identity");
        Session grouped;
        grouped.Enable(1 | 2 | 8);
        int oldHost = 0, newHost = 0;
        Hooks::VisualSlot(&first, Domain::XamlCore, 0, &a, &oldHost);
        Hooks::VisualSlot(&first, Domain::XamlCore, 0, &alias, &newHost);
        Hooks::VisualSlot(&first, Domain::XamlCore, 1, &b);
        Hooks::VisualSlot(&second, Domain::CompNode, 0, &b);
        Hooks::ReleaseVisualSlotGroup(&oldHost);
        Hooks::VisualProperty(&a, 0, 51);
        Hooks::ReleaseVisualSlotGroup(&newHost);
        Hooks::VisualProperty(&b, 0, 52);
        Hooks::VisualSlot(&first, Domain::XamlCore, 1, nullptr);
        Hooks::ReleaseVisualOwner(&second, Domain::CompNode);
        SlotDecoded groups;
        grouped.Read(OnSlot, &groups);
        Check(!groups.Failed && groups.Baselines == 2 && groups.Gaps == 2 && groups.Properties.size() == 2,
            "host-group retirement follows transferred live slot, preserves unrelated owner slots");
        Session failure;
        failure.Enable(1 | 2 | 8);
        NonWeakVisual broken;
        broken.CanonicalResult = E_OUTOFMEMORY;
        Hooks::VisualSlot(&first, Domain::CompNode, 0, &broken);
        ProtocolInfo info{};
        Check(GetProtocolInfo(1, sizeof(info), &info) == E_OUTOFMEMORY && broken.References == 1,
            "unexpected canonical identity failure remains explicit and nonretaining");
        std::puts("PASS: owner-slot partial intervals, aliases/shared owners/replacement/gaps; actual bridge ETW, synthetic native owners");
    }

    void SlotResourceGapTest()
    {
        NonWeakVisual visual;
        int owner = 0, resource = 0, group = 0;
        Session session;
        session.Enable(1 | 2 | 8 | 32);
        Hooks::Born(Domain::Resource, &resource);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual, &group);
        Hooks::VisualProperty(&visual, 0, 1);
        session.Enable(1 | 32);
        const auto queries = visual.CanonicalQueries;
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, nullptr);
        Hooks::ReleaseVisualOwner(&owner, Domain::CompNode);
        Hooks::ReleaseVisualSlotGroup(&group);
        Check(visual.CanonicalQueries == queries, "disabled slot retirement performs no QI");
        Hooks::Destroyed(Domain::CompNode, &owner);
        Hooks::Born(Domain::CompNode, &owner);
        Check(visual.Release() == 0, "unobserved old visual has no diagnostic strong retention");
        visual.~NonWeakVisual();
        new (&visual) NonWeakVisual;
        session.Enable(1 | 2 | 8 | 32);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual, &group);
        Hooks::VisualProperty(&visual, 0, 2);
        Hooks::ReleaseVisualOwner(&owner, Domain::CompNode);
        Hooks::Destroyed(Domain::Resource, &resource);
        SlotDecoded decoded;
        session.Read(OnSlot, &decoded);
        Check(!decoded.Failed && decoded.Properties.size() == 2 &&
            decoded.Properties[0] != decoded.Properties[1] && decoded.Epochs[0] == decoded.Epochs[1],
            "Resource-only gap invalidates visual fragment despite unchanged producer epoch and address reuse");
        Check(decoded.Resources.size() == 2 && decoded.Resources[0] == decoded.Resources[1],
            "continuous Resource identity survives slot-only generation reset");
        Check(decoded.Gaps == 1 && decoded.Unlinks == 1 && visual.References == 1 && visual.WeakQueries == 0,
            "no ghost owner tokens after disabled owner destruction/reuse; final visual retirement succeeds");
        std::puts("PASS: Resource-only slot gap/address reuse clears visual fragments/ghosts, preserves Resource identity");
    }
    void SlotCorrelationGenerationTest()
    {
        NonWeakVisual visual;
        VisualAlias alias(visual);
        int owner = 0;
        Session session;
        session.Enable(1 | 2);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualSlot(&owner, Domain::CompNode, 1, &alias);
        session.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &alias);
        Hooks::VisualSlot(&owner, Domain::CompNode, 1, &visual);
        Hooks::VisualProperty(&visual, 0, 1);
        session.Enable(1 | 2);
        session.Enable(1 | 2 | 8);
        Hooks::VisualSlot(&owner, Domain::CompNode, 1, &alias);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
        Hooks::VisualProperty(&visual, 0, 2);
        Hooks::VisualSlot(&owner, Domain::CompNode, 0, nullptr);
        Hooks::VisualSlot(&owner, Domain::CompNode, 1, nullptr);
        SlotDecoded decoded;
        session.Read(OnSlot, &decoded);
        Check(!decoded.Failed && decoded.Links == 2 && decoded.Unlinks == 1,
            "late visual enable/reenable announces exactly one semantic link per generation across aliases/slots");
        Check(decoded.Baselines == 1 && decoded.Properties.size() == 2 &&
            decoded.Properties[0] == decoded.Properties[1] && decoded.Epochs[0] == decoded.Epochs[1],
            "continuous lifetime observation preserves qualified visual identity across correlation generation resets");
        std::puts("PASS: correlation announcement generations are independent, deduplicated and identity-stable");
    }

    struct EdgeContextDecoded
    {
        unsigned Edges = 0, Frames = 0, Losses = 0;
        GUID Root{};
        uint64_t Dispatcher = 0;
        bool Failed = false;
    };
    void WINAPI OnEdgeContext(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != RuntimeProviderId) return;
        auto& value = *static_cast<EdgeContextDecoded*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, {L"SemanticEventId"});
            if (id == 9) ++value.Frames;
            if (id == 15) ++value.Losses;
            if (id != 5) return;
            const auto root = Field<GUID>(record, {L"RootId"});
            const auto dispatcher = Field<uint64_t>(record, {L"DispatcherId"});
            if (value.Edges == 0) { value.Root = root; value.Dispatcher = dispatcher; }
            Check(root == value.Root && root != GUID_NULL && dispatcher == value.Dispatcher && dispatcher != 0,
                "synchronous removal reuses proven edge root/dispatcher");
            Check(Field<int32_t>(record, {L"Order"}) == 2 &&
                Field<uint8_t>(record, {L"Connected"}) == (value.Edges != 1), "exact connected/removal order and pair");
            ++value.Edges;
        }
        catch (...) { value.Failed = true; }
    }
    void VisualEdgeContextTest()
    {
        NonWeakVisual parent, child;
        int parentOwner = 0, childOwner = 0, root = 0, dispatcher = 0;
        Session session;
        session.Enable(1 | 2 | 4 | 8 | 16);
        Hooks::VisualSlot(&parentOwner, Domain::CompNode, 0, &parent);
        Hooks::VisualSlot(&childOwner, Domain::CompNode, 0, &child);
        {
            Hooks::RootScope context(&root, &dispatcher, false);
            Hooks::VisualMembership(&parent, &child, 2, true);
        }
        Hooks::VisualMembership(&parent, &child, 2, false);
        {
            Hooks::RootScope context(&root, &dispatcher, false);
            Hooks::VisualMembership(&parent, &child, 2, true);
        }
        session.Enable(1 | 2 | 8 | 16);
        session.Enable(1 | 2 | 4 | 8 | 16);
        Hooks::VisualMembership(&parent, &child, 2, false);
        EdgeContextDecoded decoded;
        session.Read(OnEdgeContext, &decoded);
        Check(!decoded.Failed && decoded.Edges == 3 && decoded.Losses == 1 && decoded.Frames == 0,
            "unscoped visual removal retains proven context without creating an attempt/frame");
        std::puts("PASS: exact visual edge context survives synchronous removal without invented work/frame");
    }

    void MatchAllGenerationTest(int selected = 0)
    {
        for (bool continuous : {false, true})
        {
            if (selected == 0 || selected == 1)
            {
                NonWeakVisual visual;
                using NativeOwner = int;
                NativeOwner owner = 0;
                int resource = 0, group = 0;
                Session second, session;
                constexpr uint64_t keywords = 1 | 2 | 8 | 32;
                if (continuous) second.Enable(keywords);
                session.Enable(keywords);
                Hooks::Born(Domain::Resource, &resource);
                Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual, &group);
                Hooks::VisualProperty(&visual, 0, 1);
                session.Enable(keywords, 4, 32);
                // No hook/eligibility query executes during the entirely filtered interval.
                if (!continuous)
                {
                    owner.~NativeOwner();
                    Check(visual.Release() == 0, "MatchAll gap retains no diagnostic COM reference");
                    visual.~NonWeakVisual();
                    new (&visual) NonWeakVisual;
                }
                session.Enable(keywords);
                if (!continuous)
                {
                    new (&owner) NativeOwner(1);
                    Hooks::Born(Domain::CompNode, &owner);
                }
                Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual, &group);
                Hooks::VisualProperty(&visual, 0, 2);
                Hooks::ReleaseVisualOwner(&owner, Domain::CompNode);
                Hooks::Destroyed(Domain::Resource, &resource);
                SlotDecoded decoded;
                session.Read(OnSlot, &decoded);
                Check(!decoded.Failed && decoded.Properties.size() == 2 &&
                    (decoded.Properties[0] == decoded.Properties[1]) == continuous &&
                    decoded.Epochs[0] == decoded.Epochs[1],
                    "MatchAll Resource gap fresh fragment, or genuine second-listener continuity");
                Check(decoded.Resources.size() == 2 && decoded.Resources[0] == decoded.Resources[1] &&
                    decoded.Gaps == 1 && decoded.Unlinks == 1 && visual.WeakQueries == 0 && visual.References == 1,
                    "MatchAll gap preserves Resource identity and retires all ghost slots without COM retention");
                std::printf("PASS: MatchAll Resource-only gap continuous=%d\n", continuous);
            }
            if (selected == 0 || selected == 2)
            {
                NonWeakVisual visual;
                VisualAlias alias(visual);
                int owner = 0;
                Session second, session;
                constexpr uint64_t keywords = 1 | 2 | 8;
                if (continuous) second.Enable(keywords);
                session.Enable(keywords);
                Hooks::VisualSlot(&owner, Domain::CompNode, 0, &visual);
                Hooks::VisualSlot(&owner, Domain::CompNode, 1, &alias);
                Hooks::VisualProperty(&visual, 0, 1);
                session.Enable(keywords, 4, 2);
                session.Enable(keywords);
                Hooks::VisualSlot(&owner, Domain::CompNode, 1, &visual);
                Hooks::VisualSlot(&owner, Domain::CompNode, 0, &alias);
                Hooks::VisualProperty(&visual, 0, 2);
                Hooks::ReleaseVisualOwner(&owner, Domain::CompNode);
                SlotDecoded decoded;
                session.Read(OnSlot, &decoded);
                Check(!decoded.Failed && decoded.Links == (continuous ? 1u : 2u) &&
                    decoded.Unlinks == 1 && decoded.Baselines == 1 &&
                    decoded.Properties.size() == 2 && decoded.Properties[0] == decoded.Properties[1] &&
                    decoded.Epochs[0] == decoded.Epochs[1],
                    "MatchAll correlation renews exactly once only after actual aggregate gap, stable identity/aliases");
                std::printf("PASS: MatchAll correlation gap continuous=%d\n", continuous);
            }
            if (selected == 0 || selected == 3)
            {
                NonWeakVisual parent, child;
                int a = 0, b = 0, root = 0, dispatcher = 0;
                Session second, session;
                constexpr uint64_t keywords = 1 | 2 | 4 | 8 | 16;
                if (continuous) second.Enable(keywords);
                session.Enable(keywords);
                Hooks::VisualSlot(&a, Domain::CompNode, 0, &parent);
                Hooks::VisualSlot(&b, Domain::CompNode, 0, &child);
                {
                    Hooks::RootScope context(&root, &dispatcher, false);
                    Hooks::VisualMembership(&parent, &child, 2, true);
                }
                session.Enable(keywords, 4, 2);
                session.Enable(keywords);
                Hooks::VisualMembership(&parent, &child, 2, false);
                EdgeContextDecoded decoded;
                session.Read(OnEdgeContext, &decoded);
                Check(!decoded.Failed && decoded.Edges == (continuous ? 2u : 1u) &&
                    decoded.Losses == (continuous ? 0u : 1u) && decoded.Frames == 0,
                    "MatchAll tree gap forgets context, genuine continuously eligible listener preserves exact pair");
                std::printf("PASS: MatchAll tree gap continuous=%d\n", continuous);
            }
        }
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
int main(int argc, char** argv)
{
    Hooks::InitializeRuntime();
    struct RuntimeLifetime { ~RuntimeLifetime() { Hooks::ShutdownRuntime(); } } runtimeLifetime;
    try
    {
        if (argc == 2 && strcmp(argv[1], "--runtime-lifetime") == 0) { RuntimeLifetimeTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--layout-stack-lifetime") == 0) { LayoutStackLifetimeTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--orderly-weak-cleanup") == 0) { OrderlyWeakCleanupTest(); return 0; }
        if (argc == 3 && strcmp(argv[1], "--termination-child") == 0) { TerminationChild(argv[2]); return 0; }
        if (argc == 2 && strcmp(argv[1], "--termination") == 0) { TerminationTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--matchall-generations") == 0) { MatchAllGenerationTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--matchall-resource") == 0) { MatchAllGenerationTest(1); return 0; }
        if (argc == 2 && strcmp(argv[1], "--matchall-correlation") == 0) { MatchAllGenerationTest(2); return 0; }
        if (argc == 2 && strcmp(argv[1], "--matchall-tree") == 0) { MatchAllGenerationTest(3); return 0; }
        if (argc == 2 && strcmp(argv[1], "--visual-edge-context") == 0) { VisualEdgeContextTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--slot-resource-gap") == 0) { SlotResourceGapTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--slot-correlation-generations") == 0) { SlotCorrelationGenerationTest(); return 0; }
        if (argc == 2 && strcmp(argv[1], "--visual-slots") == 0)
        {
            VisualSlotTest();
            return 0;
        }
        if (argc == 2 && strcmp(argv[1], "--unsupported-visual") == 0)
        {
            UnsupportedVisualIsolationTest();
            return 0;
        }
        if ((argc == 3 || (argc == 4 && strcmp(argv[3], "--await-input") == 0)) &&
            (strcmp(argv[1], "--emit-fixtures") == 0 || strcmp(argv[1], "--emit-budget") == 0))
        {
            wchar_t directory[MAX_PATH]{};
            Check(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, argv[2], -1, directory, MAX_PATH) > 0, "fixture directory UTF-8");
            fixtureDirectory = directory;
            if (strcmp(argv[1], "--emit-budget") == 0)
            {
                Check(argc == 3, "budget fixtures do not wait for input");
                WireBudgetTest();
                fixtureDirectory = nullptr;
                return 0;
            }
            if (argc == 4)
            {
                std::puts("READY: scalar"); std::fflush(stdout);
                Check(std::getchar() != EOF, "fixture scalar permission");
            }
            SchemaTest();
            if (argc == 4)
            {
                std::puts("READY: arrays"); std::fflush(stdout);
                Check(std::getchar() != EOF, "fixture arrays permission");
            }
            ArraySchemaTest();
            fixtureDirectory = nullptr;
            return 0;
        }
        Check(argc == 1, "unknown harness arguments");
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
