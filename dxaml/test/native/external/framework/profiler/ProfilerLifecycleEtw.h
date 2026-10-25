// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerProtocol.h"
#include "ProfilerPostCommitOracle.h"
#include <evntrace.h>
#include <tdh.h>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <cstdio>

namespace LifecycleEtw
{
    inline void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    inline void RequireObjectsDisabledBeforeConstruction()
    {
        std::vector<BYTE> buffer(64 * 1024);
        ULONG size = 0;
        const ULONG status = EnumerateTraceGuidsEx(TraceGuidQueryInfo,
            const_cast<GUID*>(&WinUIProfiler::RuntimeProviderId), sizeof(GUID),
            buffer.data(), static_cast<ULONG>(buffer.size()), &size);
        if (status == ERROR_WMI_GUID_NOT_FOUND) return;
        Require(status == ERROR_SUCCESS && size >= sizeof(TRACE_GUID_INFO),
            "query actual local provider listeners for late-enable precondition");
        const auto* info = reinterpret_cast<const TRACE_GUID_INFO*>(buffer.data());
        size_t offset = sizeof(TRACE_GUID_INFO);
        for (ULONG i = 0; i < info->InstanceCount; ++i)
        {
            Require(offset <= size && sizeof(TRACE_PROVIDER_INSTANCE_INFO) <= size - offset,
                "bounded provider instance metadata");
            const auto* instance = reinterpret_cast<const TRACE_PROVIDER_INSTANCE_INFO*>(buffer.data() + offset);
            const size_t enabledOffset = offset + sizeof(*instance);
            Require(instance->EnableCount <= (size - enabledOffset) / sizeof(TRACE_ENABLE_INFO),
                "bounded provider listener metadata");
            const auto* enabled = reinterpret_cast<const TRACE_ENABLE_INFO*>(buffer.data() + enabledOffset);
            if (instance->Pid == GetCurrentProcessId())
                for (ULONG listener = 0; listener < instance->EnableCount; ++listener)
                    if (enabled[listener].IsEnabled && (enabled[listener].MatchAnyKeyword & 2) &&
                        (uint64_t{2} & enabled[listener].MatchAllKeyword) == enabled[listener].MatchAllKeyword &&
                        (enabled[listener].Level == 0 || enabled[listener].Level >= 4))
                        throw std::runtime_error("late-enable fixture precondition: Objects already enabled by another ETW listener before construction");
            if (i + 1 < info->InstanceCount)
            {
                Require(instance->NextOffset >= sizeof(*instance) && instance->NextOffset <= size - offset,
                    "bounded next provider instance");
                offset += instance->NextOffset;
            }
        }
        std::puts("LATE_PRECONDITION: local Objects observation disabled before construction");
    }
    template<typename T>
    T Field(EVENT_RECORD* record, const wchar_t* first, const wchar_t* second = nullptr, const wchar_t* third = nullptr)
    {
        PROPERTY_DATA_DESCRIPTOR fields[3]{};
        fields[0].PropertyName = reinterpret_cast<ULONGLONG>(first);
        fields[0].ArrayIndex = ULONG_MAX;
        fields[1].PropertyName = reinterpret_cast<ULONGLONG>(second);
        fields[1].ArrayIndex = ULONG_MAX;
        fields[2].PropertyName = reinterpret_cast<ULONGLONG>(third);
        fields[2].ArrayIndex = ULONG_MAX;
        T value{};
        const ULONG status = TdhGetProperty(record, 0, nullptr, third ? 3 : second ? 2 : 1, fields,
            sizeof(value), reinterpret_cast<BYTE*>(&value));
        if (status != ERROR_SUCCESS)
            std::fwprintf(stderr, L"TDH_FIELD_FAILED: %ls/%ls/%ls status=%lu\n",
                first, second ? second : L"", third ? third : L"", status);
        Require(status == ERROR_SUCCESS, "TDH real-hook field");
        return value;
    }
    struct Observations
    {
        uint32_t Counts[17]{};
        std::map<uint64_t, uint64_t> BirthAddresses;
        std::set<uint64_t> Destroyed;
        std::set<uint64_t> Baselines;
        struct Submitted { uint64_t Id, AttemptId; GUID Root; int64_t Timestamp; };
        std::vector<Submitted> Submissions;
        std::vector<std::pair<int64_t, uint8_t>> FrameOutcomes;
        std::vector<PostCommitFrame> PostCommitFrames;
        bool DecodePostCommit = false;
        struct Standalone { int64_t Timestamp; int32_t HResult; bool HasHResult, HasAttempt, HasSubmitted; };
        std::vector<Standalone> StandaloneCommits;
        uint64_t MeasureBegins = 0, MeasureEnds = 0, ArrangeBegins = 0, ArrangeEnds = 0;
        std::map<uint64_t, int64_t> AttemptEnds;
        std::map<uint64_t, uint8_t> Domains;
        std::vector<std::pair<uint64_t, GUID>> PropertyObjects;
        struct PropertyValue
        {
            uint64_t ObjectId;
            uint8_t PreviousKind, PreviousUnit, CurrentKind, CurrentUnit;
            double Number;
        };
        std::vector<PropertyValue> PropertyValues;
        struct ScopedEdge
        {
            uint64_t Child, Dispatcher;
            GUID Epoch, Root;
            int64_t Timestamp;
            bool Connected;
            uint8_t Membership = 0, Relationship = 0, Domain = 0;
            uint64_t Parent = 0, Sequence = 0;
            uint32_t Thread = 0;
        };
        std::vector<ScopedEdge> ScopedEdges;
        struct VisualEdge { uint64_t Parent, Child; int32_t Order; bool Connected; int64_t Timestamp; };
        std::vector<VisualEdge> VisualEdges;
        std::map<uint64_t, std::set<uint64_t>> VisualOwners;
        std::map<uint64_t, std::vector<std::pair<int64_t, uint64_t>>> VisualAddresses;
        std::vector<int64_t> RejectedHandInInsertions;
        std::vector<std::pair<int64_t, std::string>> LossReasons;
        struct LossRecord
        {
            GUID Epoch;
            uint64_t Sequence, Count;
            int64_t Timestamp, Begin, End;
            bool HasCount;
        };
        std::vector<LossRecord> Losses;
        struct DeviceRecord
        {
            GUID Producer, Epoch;
            uint64_t Sequence;
            uint8_t Operation, Coverage;
        };
        std::vector<DeviceRecord> Devices;
        struct ResourceRecord
        {
            GUID Producer, Epoch, DeviceEpoch;
            uint64_t Id, Sequence;
            uint32_t Width, Height;
            uint8_t Operation, Kind, Coverage;
            bool HasBacking, HasBytes;
            int64_t Timestamp;
            GUID OwnerEpoch;
            uint64_t OwnerId;
            uint8_t OwnerDomain;
            bool HasOwner;
        };
        std::vector<ResourceRecord> Resources;
        struct AllocationRecord
        {
            uint64_t Id, Sequence, Previous, Requested, Actual;
            GUID Epoch, Producer;
            int64_t Timestamp;
            uint8_t Operation, Coverage, Provenance;
            bool HasPrevious, HasActual, HasBaseline;
        };
        std::vector<AllocationRecord> Allocations;
        ULONG EventsLost = 0, LogBuffersLost = 0, RealTimeBuffersLost = 0;
        struct LifecycleRecord
        {
            uint16_t Event;
            uint64_t Sequence, Object;
            uint8_t Operation, Domain, HasType, Schema;
            GUID Epoch;
            int64_t Timestamp;
        };
        std::vector<LifecycleRecord> LifecycleRecords;
        bool Failed = false;
    };
    inline void WINAPI Decode(EVENT_RECORD* record)
    {
        if (record->EventHeader.ProviderId != WinUIProfiler::RuntimeProviderId ||
            record->EventHeader.ProcessId != GetCurrentProcessId()) return;
        auto& observations = *static_cast<Observations*>(record->UserContext);
        try
        {
            const auto id = Field<uint16_t>(record, L"SemanticEventId");
            Require(id > 0 && id <= 16, "real-hook catalog ID");
            ++observations.Counts[id];
            if (id == 2 || id == 3 || id == 16)
                observations.LifecycleRecords.push_back({
                    id, Field<uint64_t>(record, L"SourceSequence"),
                    id == 3 ? Field<uint64_t>(record, L"TypeId") : Field<uint64_t>(record, L"Object", L"RuntimeObjectId"),
                    id == 2 ? Field<uint8_t>(record, L"Operation") : uint8_t{255},
                    id == 3 ? uint8_t{255} : Field<uint8_t>(record, L"Object", L"Domain"),
                    id == 2 ? Field<uint8_t>(record, L"TypeId", L"Present") : uint8_t{0},
                    Field<uint8_t>(record, L"EventSchemaVersion"),
                    Field<GUID>(record, L"ProducerEpoch"), record->EventHeader.TimeStamp.QuadPart});
            if (id == 5)
                observations.ScopedEdges.push_back({
                    Field<uint64_t>(record, L"Child", L"RuntimeObjectId"),
                    Field<uint64_t>(record, L"DispatcherId"),
                    Field<GUID>(record, L"ProducerEpoch"), Field<GUID>(record, L"RootId"),
                    record->EventHeader.TimeStamp.QuadPart, Field<uint8_t>(record, L"Connected") != 0,
                    Field<uint8_t>(record, L"Membership"), Field<uint8_t>(record, L"Relationship"),
                    Field<uint8_t>(record, L"Child", L"Domain"),
                    Field<uint8_t>(record, L"Parent", L"Present") ?
                        Field<uint64_t>(record, L"Parent", L"Value", L"RuntimeObjectId") : 0,
                    Field<uint64_t>(record, L"SourceSequence"), record->EventHeader.ThreadId});
            if (id == 2)
            {
                const auto object = Field<uint64_t>(record, L"Object", L"RuntimeObjectId");
                const auto operation = Field<uint8_t>(record, L"Operation");
                const auto domain = Field<uint8_t>(record, L"Object", L"Domain");
                const auto known = observations.Domains.find(object);
                Require(known == observations.Domains.end() || known->second == domain, "lifetime domain cannot change");
                observations.Domains[object] = domain;
                if (domain == 3 && Field<uint8_t>(record, L"Address", L"Present"))
                    observations.VisualAddresses[Field<uint64_t>(record, L"Address", L"Value")].push_back(
                        {record->EventHeader.TimeStamp.QuadPart, object});
                if (operation == 0)
                {
                    Require(observations.BirthAddresses.count(object) == 0, "lifetime birth only once");
                    observations.BirthAddresses.emplace(object, Field<uint64_t>(record, L"Address", L"Value"));
                    Require(Field<uint8_t>(record, L"TypeId", L"Present") == 0, "base constructor cannot resolve derived type");
                }
                else if (operation == 1) observations.Destroyed.insert(object);
                else observations.Baselines.insert(object);
            }
            else if (id == 5 && Field<uint8_t>(record, L"Relationship") == 8)
            {
                observations.VisualEdges.push_back({
                    Field<uint64_t>(record, L"Parent", L"Value", L"RuntimeObjectId"),
                    Field<uint64_t>(record, L"Child", L"RuntimeObjectId"),
                    Field<int32_t>(record, L"Order"), Field<uint8_t>(record, L"Connected") != 0,
                    record->EventHeader.TimeStamp.QuadPart});
            }
            else if (id == 7 && Field<uint8_t>(record, L"Kind") == 2 && Field<uint8_t>(record, L"Linked"))
                observations.VisualOwners[Field<uint64_t>(record, L"To", L"RuntimeObjectId")].insert(
                    Field<uint64_t>(record, L"From", L"RuntimeObjectId"));
            else if (id == 8)
            {
                const auto kind = Field<uint8_t>(record, L"Kind");
                const auto phase = Field<uint8_t>(record, L"Phase");
                if (kind == 2) ++(phase == 0 ? observations.MeasureBegins : observations.MeasureEnds);
                if (kind == 3) ++(phase == 0 ? observations.ArrangeBegins : observations.ArrangeEnds);
            }
            else if (id == 9)
            {
                const auto outcome = Field<uint8_t>(record, L"Outcome");
                observations.FrameOutcomes.emplace_back(record->EventHeader.TimeStamp.QuadPart, outcome);
                if (observations.DecodePostCommit)
                {
                    PostCommitFrame frame;
                    frame.Epoch = Field<GUID>(record, L"ProducerEpoch");
                    frame.Sequence = Field<uint64_t>(record, L"SourceSequence");
                    frame.Timestamp = record->EventHeader.TimeStamp.QuadPart;
                    frame.Outcome = outcome;
                    frame.HasAttempt = Field<uint8_t>(record, L"Attempt", L"Present") != 0;
                    frame.HasSubmitted = Field<uint8_t>(record, L"SubmittedFrameId", L"Present") != 0;
                    frame.HasResult = Field<uint8_t>(record, L"HResult", L"Present") != 0;
                    if (frame.HasAttempt)
                    {
                        frame.AttemptEpoch = Field<GUID>(record, L"Attempt", L"Value", L"ProducerEpoch");
                        frame.Attempt = Field<uint64_t>(record, L"Attempt", L"Value", L"AttemptId");
                        frame.Root = Field<GUID>(record, L"Attempt", L"Value", L"RootId");
                        frame.Dispatcher = Field<uint64_t>(record, L"Attempt", L"Value", L"DispatcherId");
                    }
                    if (frame.HasSubmitted) frame.Submitted = Field<uint64_t>(record, L"SubmittedFrameId", L"Value");
                    if (frame.HasResult) frame.Result = Field<int32_t>(record, L"HResult", L"Value");
                    observations.PostCommitFrames.push_back(frame);
                }
                if (outcome == 1)
                {
                    observations.Submissions.push_back({
                        Field<uint64_t>(record, L"SubmittedFrameId", L"Value"),
                        Field<uint64_t>(record, L"Attempt", L"Value", L"AttemptId"),
                        Field<GUID>(record, L"Attempt", L"Value", L"RootId"),
                        record->EventHeader.TimeStamp.QuadPart});
                }
                else if (outcome == 3)
                {
                    const bool hasResult = Field<uint8_t>(record, L"HResult", L"Present") != 0;
                    observations.StandaloneCommits.push_back({
                        record->EventHeader.TimeStamp.QuadPart,
                        hasResult ? Field<int32_t>(record, L"HResult", L"Value") : 0, hasResult,
                        Field<uint8_t>(record, L"Attempt", L"Present") != 0,
                        Field<uint8_t>(record, L"SubmittedFrameId", L"Present") != 0});
                }
                else if (outcome == 2 || outcome == 4)
                    observations.AttemptEnds.emplace(
                        Field<uint64_t>(record, L"Attempt", L"Value", L"AttemptId"), record->EventHeader.TimeStamp.QuadPart);
            }
            else if (id == 12)
            {
                Require(Field<uint32_t>(record, L"TargetProcess", L"ProcessId") == GetCurrentProcessId() &&
                    Field<uint8_t>(record, L"EventSchemaVersion") == 1,
                    "actual allocator records identify this process and the supported wire schema");
                const bool hasPrevious = Field<uint8_t>(record, L"PreviousAllocationId", L"Present") != 0;
                const bool hasActual = Field<uint8_t>(record, L"ActualBytes", L"Present") != 0;
                observations.Allocations.push_back({
                    Field<uint64_t>(record, L"AllocationId"), Field<uint64_t>(record, L"SourceSequence"),
                    hasPrevious ? Field<uint64_t>(record, L"PreviousAllocationId", L"Value") : 0,
                    Field<uint64_t>(record, L"RequestedBytes"),
                    hasActual ? Field<uint64_t>(record, L"ActualBytes", L"Value") : 0,
                    Field<GUID>(record, L"AllocatorEpoch"), Field<GUID>(record, L"ProducerEpoch"),
                    record->EventHeader.TimeStamp.QuadPart, Field<uint8_t>(record, L"Operation"),
                    Field<uint8_t>(record, L"Quality", L"Coverage"),
                    Field<uint8_t>(record, L"Quality", L"Kind"), hasPrevious, hasActual,
                    Field<uint8_t>(record, L"BaselineCaptureId", L"Present") != 0});
            }
            else if (id == 10)
            {
                observations.Devices.push_back({
                    Field<GUID>(record, L"ProducerEpoch"), Field<GUID>(record, L"DeviceEpoch"),
                    Field<uint64_t>(record, L"SourceSequence"), Field<uint8_t>(record, L"Operation"),
                    Field<uint8_t>(record, L"Quality", L"Coverage")});
            }
            else if (id == 11)
            {
                const bool hasOwner = Field<uint8_t>(record, L"Owner", L"Present") != 0;
                observations.Resources.push_back({
                    Field<GUID>(record, L"ProducerEpoch"), Field<GUID>(record, L"Resource", L"ProducerEpoch"),
                    Field<GUID>(record, L"DeviceEpoch"), Field<uint64_t>(record, L"Resource", L"RuntimeObjectId"),
                    Field<uint64_t>(record, L"SourceSequence"),
                    Field<uint8_t>(record, L"WidthPixels", L"Present") ?
                        Field<uint32_t>(record, L"WidthPixels", L"Value") : 0,
                    Field<uint8_t>(record, L"HeightPixels", L"Present") ?
                        Field<uint32_t>(record, L"HeightPixels", L"Value") : 0,
                    Field<uint8_t>(record, L"Operation"), Field<uint8_t>(record, L"Classification", L"Kind"),
                    Field<uint8_t>(record, L"Quality", L"Coverage"),
                    Field<uint8_t>(record, L"BackingAllocationId", L"Present") != 0,
                    Field<uint8_t>(record, L"AllocationBytes", L"Present") != 0,
                    record->EventHeader.TimeStamp.QuadPart,
                    hasOwner ? Field<GUID>(record, L"Owner", L"Value", L"ProducerEpoch") : GUID_NULL,
                    hasOwner ? Field<uint64_t>(record, L"Owner", L"Value", L"RuntimeObjectId") : 0,
                    hasOwner ? Field<uint8_t>(record, L"Owner", L"Value", L"Domain") : uint8_t{255},
                    hasOwner});
            }
            else if (id == 15)
            {
                const bool hasCount = Field<uint8_t>(record, L"LostEvents", L"Present") != 0;
                observations.Losses.push_back({
                    Field<GUID>(record, L"ProducerEpoch"), Field<uint64_t>(record, L"SourceSequence"),
                    hasCount ? Field<uint64_t>(record, L"LostEvents", L"Value") : 0,
                    record->EventHeader.TimeStamp.QuadPart,
                    Field<int64_t>(record, L"Interval", L"StartQpc"),
                    Field<int64_t>(record, L"Interval", L"EndQpc"), hasCount});
                PROPERTY_DATA_DESCRIPTOR reason{};
                reason.PropertyName = reinterpret_cast<ULONGLONG>(L"Reason");
                reason.ArrayIndex = ULONG_MAX;
                ULONG size = 0;
                Require(TdhGetPropertySize(record, 0, nullptr, 1, &reason, &size) == ERROR_SUCCESS &&
                    size > 0 && size <= 4096, "bounded scoped loss reason");
                std::vector<char> text(size + 1);
                Require(TdhGetProperty(record, 0, nullptr, 1, &reason, size,
                    reinterpret_cast<BYTE*>(text.data())) == ERROR_SUCCESS, "scoped loss reason");
                observations.LossReasons.emplace_back(record->EventHeader.TimeStamp.QuadPart, text.data());
                if (std::string(text.data()) ==
                    "Native hand-in InsertAtTop rejected: HRESULT=0x80070057; no VisualChild edge")
                    observations.RejectedHandInInsertions.push_back(record->EventHeader.TimeStamp.QuadPart);
            }
            else if (id == 16)
            {
                observations.PropertyObjects.emplace_back(Field<uint64_t>(record, L"Object", L"RuntimeObjectId"),
                    Field<GUID>(record, L"Object", L"ProducerEpoch"));
                observations.PropertyValues.push_back({
                    observations.PropertyObjects.back().first,
                    Field<uint8_t>(record, L"Previous", L"Kind"), Field<uint8_t>(record, L"Previous", L"Unit"),
                    Field<uint8_t>(record, L"Current", L"Kind"), Field<uint8_t>(record, L"Current", L"Unit"),
                    Field<double>(record, L"Current", L"Number")});
            }
        }
        catch (const std::exception& error) { std::fprintf(stderr, "ETW_DECODE_FAILED: %s\n", error.what()); observations.Failed = true; }
        catch (...) { std::fputs("ETW_DECODE_FAILED: unexpected native exception\n", stderr); observations.Failed = true; }
    }
    class Session
    {
    public:
        explicit Session(bool retain = false, bool decodePostCommit = false) :
            m_retain(retain), m_decodePostCommit(decodePostCommit)
        {
            static volatile LONG serial = 0;
            swprintf_s(m_name, L"WinUIProfilerRealHooks-%lu-%llu-%ld",
                GetCurrentProcessId(), GetTickCount64(), InterlockedIncrement(&serial));
            m_properties.Header.Wnode.BufferSize = sizeof(m_properties);
            m_properties.Header.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
            m_properties.Header.Wnode.ClientContext = 1;
            m_properties.Header.LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
            m_properties.Header.LoggerNameOffset = offsetof(Properties, Name);
            m_properties.Header.LogFileNameOffset = offsetof(Properties, File);
            wchar_t executable[MAX_PATH]{};
            Require(GetModuleFileNameW(nullptr, executable, ARRAYSIZE(executable)) != 0, "host path");
            wchar_t* slash = wcsrchr(executable, L'\\');
            Require(slash != nullptr, "host path separator");
            slash[1] = 0;
            swprintf_s(m_properties.File, L"%ls%ls.etl", executable, m_name);
            const ULONG result = StartTraceW(&m_handle, m_name, &m_properties.Header);
            std::printf("OWNED_SESSION: %ls StartTrace=%lu\n", m_name, result); std::fflush(stdout);
            Require(result == ERROR_SUCCESS, "start owned real-hook ETW session");
        }
        ~Session()
        {
            if (m_handle) ControlTraceW(m_handle, m_name, &m_properties.Header, EVENT_TRACE_CONTROL_STOP);
            if (m_retain) std::wprintf(L"RETAINED_ETL: %s\n", m_properties.File);
            else DeleteFileW(m_properties.File);
        }
        void Enable(uint64_t keywords, uint64_t matchAll = 0)
        {
            Require(EnableTraceEx2(m_handle, &WinUIProfiler::RuntimeProviderId,
                EVENT_CONTROL_CODE_ENABLE_PROVIDER, 4, keywords, matchAll, 5000, nullptr) == ERROR_SUCCESS, "enable loaded product provider");
        }
        void Disable()
        {
            Require(EnableTraceEx2(m_handle, &WinUIProfiler::RuntimeProviderId,
                EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 5000, nullptr) == ERROR_SUCCESS, "disable only owned listener");
        }
        void Flush()
        {
            Require(ControlTraceW(m_handle, m_name, &m_properties.Header, EVENT_TRACE_CONTROL_FLUSH) ==
                ERROR_SUCCESS, "flush owned real-hook transport");
        }
        Observations Read()
        {
            Require(ControlTraceW(m_handle, m_name, &m_properties.Header, EVENT_TRACE_CONTROL_STOP) == ERROR_SUCCESS, "stop owned real-hook transport");
            m_handle = 0;
            std::printf("ETW_COUNTERS: EventsLost=%lu LogBuffersLost=%lu RealTimeBuffersLost=%lu\n",
                m_properties.Header.EventsLost, m_properties.Header.LogBuffersLost, m_properties.Header.RealTimeBuffersLost);
            Observations observations;
            observations.DecodePostCommit = m_decodePostCommit;
            observations.EventsLost = m_properties.Header.EventsLost;
            observations.LogBuffersLost = m_properties.Header.LogBuffersLost;
            observations.RealTimeBuffersLost = m_properties.Header.RealTimeBuffersLost;
            EVENT_TRACE_LOGFILEW log{};
            log.LogFileName = m_properties.File;
            log.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
            log.EventRecordCallback = Decode;
            log.Context = &observations;
            TRACEHANDLE reader = OpenTraceW(&log);
            Require(reader != INVALID_PROCESSTRACE_HANDLE, "open real-hook ETL");
            const ULONG result = ProcessTrace(&reader, 1, nullptr, nullptr);
            CloseTrace(reader);
            Require(result == ERROR_SUCCESS && !observations.Failed, "decode real-hook ETL");
            std::printf("OBSERVED: lifetimes=%u types=%u associations=%u trees=%u visuals=%u correlations=%u boundaries=%u frames=%u properties=%u\n",
                observations.Counts[2], observations.Counts[3], observations.Counts[4], observations.Counts[5],
                observations.Counts[6], observations.Counts[7], observations.Counts[8], observations.Counts[9], observations.Counts[16]);
            return observations;
        }
    private:
        struct Properties { EVENT_TRACE_PROPERTIES Header{}; wchar_t Name[128]{}; wchar_t File[MAX_PATH]{}; } m_properties;
        TRACEHANDLE m_handle = 0;
        wchar_t m_name[128]{};
        bool m_retain = false;
        bool m_decodePostCommit = false;
    };
}
