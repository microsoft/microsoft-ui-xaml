// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerTracing.h"
#include <TraceLoggingProvider.h>
#include <cstring>
#include <new>

namespace WinUIProfiler::Wire
{
    // Public SDK TraceLogging wire format (TraceLoggingProvider.h): tightly
    // packed typed metadata/data, including UINT16-counted struct arrays.
    // A full capture schema exceeds TraceLoggingWrite's 99-argument limit.
    class Encoder final
    {
    public:
        explicit Encoder(const char* name) noexcept
        {
            m_buffer = static_cast<BYTE*>(HeapAlloc(GetProcessHeap(), 0, MetadataCapacity + MaximumWireBytes));
            if (!m_buffer) { m_error = ERROR_NOT_ENOUGH_MEMORY; return; }
            const BYTE header[3]{};
            Metadata(header, sizeof(header));
            Metadata(name, strlen(name) + 1);
        }
        ~Encoder() noexcept { if (m_buffer) HeapFree(GetProcessHeap(), 0, m_buffer); }
        Encoder(const Encoder&) = delete;
        Encoder& operator=(const Encoder&) = delete;
        bool WritingData() const noexcept { return m_dataEnabled && m_error == ERROR_SUCCESS; }

        void Struct(const char* name, uint8_t fields) noexcept { Field(name, 24, fields); }
        void U8(const char* name, uint8_t value, uint8_t output = 0) noexcept { Field(name, TlgInUINT8, output); Integer(value, 1); }
        void U16(const char* name, uint16_t value) noexcept { Field(name, TlgInUINT16); Integer(value, 2); }
        void U32(const char* name, uint32_t value) noexcept { Field(name, TlgInUINT32); Integer(value, 4); }
        void U64(const char* name, uint64_t value) noexcept { Field(name, TlgInUINT64); Integer(value, 8); }
        void I32(const char* name, int32_t value) noexcept { Field(name, TlgInINT32); Integer(static_cast<uint32_t>(value), 4); }
        void I64(const char* name, int64_t value) noexcept { Field(name, TlgInINT64); Integer(static_cast<uint64_t>(value), 8); }
        void Bool(const char* name, bool value) noexcept { U8(name, value ? 1 : 0, TlgOutBOOLEAN); }
        void F64(const char* name, double value) noexcept
        {
            Field(name, TlgInDOUBLE);
            uint64_t bits;
            static_assert(sizeof(bits) == sizeof(value));
            memcpy(&bits, &value, sizeof(bits));
            Integer(bits, 8);
        }
        void Guid(const char* name, const GUID& value) noexcept
        {
            Field(name, TlgInGUID);
            Integer(value.Data1, 4); Integer(value.Data2, 2); Integer(value.Data3, 2);
            Data(value.Data4, sizeof(value.Data4));
        }
        void Text(const char* name, const char* value) noexcept
        {
            Field(name, TlgInANSISTRING, TlgOutUTF8);
            if (m_dataEnabled)
            {
                const char* text = value ? value : "";
                const size_t length = strnlen_s(text, 1025);
                if (length > 1024) { m_error = ERROR_INVALID_DATA; return; }
                Data(text, length + 1);
            }
        }
        void NullableText(const char* name, const char* value) noexcept
        {
            Struct(name, 2); Bool("Present", value != nullptr); Text("Value", value);
        }
        template<typename T, typename Function>
        void OptionalField(const char* name, const Optional<T>& value, Function encode) noexcept
        {
            Struct(name, 2);
            Bool("Present", value.Present);
            encode(*this, "Value", value.Present ? value.Value : T{});
        }
        template<typename T, typename Function>
        void ArrayField(const char* name, const Array<T>& values, uint8_t fields, Function encode) noexcept
        {
            Field(name, 24 | TlgInVcount, fields);
            Integer(values.Count, 2);
            const bool metadata = m_metadataEnabled, data = m_dataEnabled;
            const T empty{};
            // Even an empty array has its complete typed element schema.
            const uint32_t iterations = values.Count ? values.Count : 1;
            for (uint32_t i = 0; i < iterations && m_error == ERROR_SUCCESS; ++i)
            {
                m_metadataEnabled = metadata && i == 0;
                m_dataEnabled = data && values.Count != 0;
                encode(*this, values.Count ? values.Items[i] : empty);
            }
            m_metadataEnabled = metadata;
            m_dataEnabled = data;
        }
        HRESULT Send(REGHANDLE handle, const BYTE* traits, ULONG traitsBytes, uint64_t keyword, UCHAR level) noexcept
        {
            if (m_error) return HRESULT_FROM_WIN32(m_error);
            if (m_metadataSize + m_dataSize + traitsBytes > MaximumWireBytes) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            m_buffer[0] = static_cast<BYTE>(m_metadataSize);
            m_buffer[1] = static_cast<BYTE>(m_metadataSize >> 8);
            EVENT_DATA_DESCRIPTOR data[3]{};
            EventDataDescCreate(&data[0], traits, traitsBytes);
            data[0].Reserved = EVENT_DATA_DESCRIPTOR_TYPE_PROVIDER_METADATA;
            EventDataDescCreate(&data[1], m_buffer, m_metadataSize);
            data[1].Reserved = EVENT_DATA_DESCRIPTOR_TYPE_EVENT_METADATA;
            EventDataDescCreate(&data[2], m_buffer + MetadataCapacity, m_dataSize);
            EVENT_DESCRIPTOR descriptor{};
            descriptor.Channel = 11;
            descriptor.Level = level;
            descriptor.Keyword = keyword;
            return HRESULT_FROM_WIN32(EventWriteTransfer(handle, &descriptor, nullptr, nullptr, 3, data));
        }
    private:
        static constexpr uint32_t MetadataCapacity = 8192;
        BYTE* m_buffer = nullptr;
        uint32_t m_metadataSize = 0, m_dataSize = 0;
        ULONG m_error = ERROR_SUCCESS;
        bool m_metadataEnabled = true, m_dataEnabled = true;

        void Metadata(const void* bytes, size_t size) noexcept
        {
            if (!m_metadataEnabled || m_error) return;
            if (size > MetadataCapacity - m_metadataSize) { m_error = ERROR_BUFFER_OVERFLOW; return; }
            memcpy(m_buffer + m_metadataSize, bytes, size);
            m_metadataSize += static_cast<uint32_t>(size);
        }
        void Data(const void* bytes, size_t size) noexcept
        {
            if (!m_dataEnabled || m_error) return;
            if (size > MaximumWireBytes - m_dataSize) { m_error = ERROR_BUFFER_OVERFLOW; return; }
            memcpy(m_buffer + MetadataCapacity + m_dataSize, bytes, size);
            m_dataSize += static_cast<uint32_t>(size);
        }
        void Field(const char* name, uint8_t type, uint8_t output = 0) noexcept
        {
            Metadata(name, strlen(name) + 1);
            const BYTE types[] = {static_cast<BYTE>(type | (output ? 128 : 0)), output};
            Metadata(types, output ? 2 : 1);
        }
        void Integer(uint64_t value, uint32_t bytes) noexcept
        {
            BYTE packed[8];
            for (uint32_t i = 0; i < bytes; ++i) packed[i] = static_cast<BYTE>(value >> (i * 8));
            Data(packed, bytes);
        }
    };

    inline void QualityField(Encoder& e, const char* name, const Quality& value) noexcept
    {
        e.Struct(name, 3); e.U8("Kind", value.Kind); e.U8("Coverage", value.Coverage); e.NullableText("Reason", value.Reason);
    }
    inline void ClassificationField(Encoder& e, const char* name, const Classification& value) noexcept
    {
        e.Struct(name, 3); e.U8("Kind", value.Kind); e.U8("Evidence", value.Evidence); e.Text("Source", value.Source);
    }
    inline void ObjectField(Encoder& e, const char* name, const ObjectIdentity& value) noexcept
    {
        e.Struct(name, 3); e.U8("Domain", static_cast<uint8_t>(value.IdentityDomain));
        e.Guid("ProducerEpoch", value.Epoch); e.U64("RuntimeObjectId", value.Id);
    }
    inline void AttemptField(Encoder& e, const char* name, const Attempt& value) noexcept
    {
        e.Struct(name, 4); e.Guid("ProducerEpoch", value.Epoch); e.U64("AttemptId", value.Id);
        e.Guid("RootId", value.RootId); e.U64("DispatcherId", value.DispatcherId);
    }
    inline void CauseField(Encoder& e, const Cause& value) noexcept
    {
        e.Struct("Cause", 3); e.U8("Kind", value.Kind);
        e.OptionalField("Attempt", value.Reference, AttemptField); e.NullableText("Reason", value.Reason);
    }
    inline void ValueField(Encoder& e, const char* name, const Value& value) noexcept
    {
        e.Struct(name, 7); e.U8("Kind", value.Kind); e.U8("Unit", value.Unit);
        e.I64("Integer", value.Integer); e.F64("Number", value.Number); e.Bool("Boolean", value.Boolean);
        e.Text("Text", value.Text); QualityField(e, "Quality", value.DataQuality);
    }
    inline void U64Field(Encoder& e, const char* name, uint64_t value) noexcept { e.U64(name, value); }
    inline void U32Field(Encoder& e, const char* name, uint32_t value) noexcept { e.U32(name, value); }
    inline void I32Field(Encoder& e, const char* name, int32_t value) noexcept { e.I32(name, value); }
    inline void GuidField(Encoder& e, const char* name, const GUID& value) noexcept { e.Guid(name, value); }
    inline void DomainField(Encoder& e, const DomainReference& value) noexcept
    {
        e.U16("EventId", value.EventId); e.U8("EventVersion", value.Version);
    }
    inline void CutField(Encoder& e, const char* name, const SettledCut& value) noexcept
    {
        e.Struct(name, 3); e.Guid("CutId", value.CutId);
        e.U64("ReservedThrough", value.ReservedThrough); e.U64("SettledThrough", value.SettledThrough);
    }
    inline void Encode(Encoder& e, const ObjectLifetime& p) noexcept
    {
        ObjectField(e, "Object", p.Object); e.U8("Operation", p.Operation);
        e.OptionalField("TypeId", p.TypeId, U64Field); e.OptionalField("Address", p.Address, U64Field);
        QualityField(e, "Quality", p.DataQuality); e.OptionalField("Classification", p.TypeClassification, ClassificationField); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const TypeResolved& p) noexcept
    {
        e.U64("TypeId", p.TypeId); e.Text("Name", p.Name); e.OptionalField("Classification", p.TypeClassification, ClassificationField);
    }
    inline void Encode(Encoder& e, const CorePeerAssociation& p) noexcept
    {
        ObjectField(e, "Core", p.Core); ObjectField(e, "Peer", p.Peer); e.Bool("Linked", p.Linked); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const TreeEdge& p) noexcept
    {
        e.OptionalField("Parent", p.Parent, ObjectField); ObjectField(e, "Child", p.Child);
        e.U8("Relationship", p.Relationship); e.I32("Order", p.Order); e.Guid("RootId", p.RootId);
        e.U64("DispatcherId", p.DispatcherId); e.Bool("Connected", p.Connected); CauseField(e, p.ChangeCause); e.U8("Membership", p.Membership);
    }
    inline void Encode(Encoder& e, const VisualDelta& p) noexcept
    {
        ObjectField(e, "Visual", p.Visual); e.U8("Property", p.Property); ValueField(e, "Value", p.PropertyValue); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const Correlation& p) noexcept
    {
        ObjectField(e, "From", p.From); ObjectField(e, "To", p.To);
        e.U8("Kind", p.Kind); e.Bool("Linked", p.Linked); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const Boundary& p) noexcept
    {
        e.U8("Kind", p.Kind); e.U8("Phase", p.Phase); e.U64("OperationId", p.OperationId);
        e.OptionalField("HResult", p.HResult, I32Field); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const Frame& p) noexcept
    {
        e.OptionalField("Attempt", p.Reference, AttemptField); e.OptionalField("SubmittedFrameId", p.SubmittedFrameId, U64Field);
        e.U8("Outcome", p.Outcome); e.OptionalField("HResult", p.HResult, I32Field);
    }
    inline void Encode(Encoder& e, const Device& p) noexcept
    {
        e.Guid("DeviceEpoch", p.DeviceEpoch); e.U8("Operation", p.Operation); e.Text("Api", p.Api); QualityField(e, "Quality", p.DataQuality);
    }
    inline void Encode(Encoder& e, const Resource& p) noexcept
    {
        ObjectField(e, "Resource", p.Object); e.U8("Operation", p.Operation); e.Guid("DeviceEpoch", p.DeviceEpoch);
        e.OptionalField("BackingAllocationId", p.BackingAllocationId, GuidField); e.OptionalField("AllocationBytes", p.AllocationBytes, U64Field);
        e.U8("SizeProvenance", p.SizeProvenance); e.OptionalField("Owner", p.Owner, ObjectField);
        e.OptionalField("WidthPixels", p.WidthPixels, U32Field); e.OptionalField("HeightPixels", p.HeightPixels, U32Field);
        QualityField(e, "Quality", p.DataQuality); ClassificationField(e, "Classification", p.ResourceClassification); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const Allocation& p) noexcept
    {
        e.U64("AllocationId", p.AllocationId); e.U8("Operation", p.Operation);
        e.OptionalField("PreviousAllocationId", p.PreviousAllocationId, U64Field);
        e.U64("RequestedBytes", p.RequestedBytes); e.OptionalField("ActualBytes", p.ActualBytes, U64Field);
        e.Guid("AllocatorEpoch", p.AllocatorEpoch); e.OptionalField("BaselineCaptureId", p.BaselineCaptureId, GuidField);
        QualityField(e, "Quality", p.DataQuality); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const CaptureBoundary& p) noexcept
    {
        e.Guid("CaptureId", p.CaptureId); e.U8("Kind", p.Kind); e.U8("Phase", p.Phase);
        e.OptionalField("RootId", p.RootId, GuidField); e.OptionalField("DispatcherId", p.DispatcherId, U64Field);
        e.ArrayField("Domains", p.Domains, 5, [](Encoder& item, const DomainRange& range) noexcept
        {
            item.Struct("Domain", 2); DomainField(item, range.Domain);
            item.OptionalField("BeforeSequence", range.BeforeSequence, U64Field);
            item.OptionalField("ThroughSequence", range.ThroughSequence, U64Field);
            QualityField(item, "Quality", range.DataQuality); item.OptionalField("SettledCut", range.Cut, CutField);
        });
        e.OptionalField("ChunkCount", p.ChunkCount, U32Field); QualityField(e, "Quality", p.DataQuality);
    }
    inline void Encode(Encoder& e, const SnapshotObject& p) noexcept
    {
        ObjectField(e, "Key", p.Key); e.NullableText("TypeName", p.TypeName);
        e.OptionalField("Address", p.Address, U64Field); ValueField(e, "Size", p.Size);
        e.OptionalField("Classification", p.TypeClassification, ClassificationField);
    }
    inline void Encode(Encoder& e, const OwnershipEdge& p) noexcept
    {
        ObjectField(e, "From", p.From); ObjectField(e, "To", p.To); e.Text("Relationship", p.Relationship);
        e.U8("Evidence", p.Evidence); QualityField(e, "Quality", p.DataQuality);
    }
    // TDH includes a hidden count property for each variable array and rejects
    // schemas exceeding 128 properties. Typed shared rows keep the complete
    // capture schema within that limit; indices are 1-based, zero means null.
    template<typename T>
    struct RowTable
    {
        static constexpr uint32_t Capacity = 4096;
        const T* Items[Capacity];
        uint32_t Count = 0;
        bool Overflow = false;
        uint32_t Add(Encoder& e, const T& value) noexcept
        {
            if (!e.WritingData()) return 0;
            if (Count == Capacity) { Overflow = true; return 0; }
            Items[Count++] = &value;
            return Count;
        }
        template<typename Function>
        void Write(Encoder& e, const char* name, uint8_t fields, Function encode) noexcept
        {
            e.ArrayField(name, Array<const T*>{Items, Count}, fields, [&](Encoder& item, const T* value) noexcept
            {
                const T empty{};
                encode(item, value ? *value : empty);
            });
        }
    };
    struct CaptureTables
    {
        RowTable<ObjectIdentity> Keys;
        RowTable<Value> Values;
        RowTable<Classification> Classifications;
        RowTable<Cause> Causes;
        RowTable<Quality> Qualities;
    };
    inline HRESULT EncodeCapture(Encoder& e, const CaptureChunk& p) noexcept
    {
        struct Storage
        {
            CaptureTables* Tables;
            ~Storage() noexcept { if (Tables) { Tables->~CaptureTables(); HeapFree(GetProcessHeap(), 0, Tables); } }
        } storage{static_cast<CaptureTables*>(HeapAlloc(GetProcessHeap(), 0, sizeof(CaptureTables)))};
        if (!storage.Tables) return E_OUTOFMEMORY;
        auto& t = *new (storage.Tables) CaptureTables;
        e.Guid("CaptureId", p.CaptureId); e.U32("ChunkIndex", p.ChunkIndex); e.Guid("RootId", p.RootId); e.U64("DispatcherId", p.DispatcherId);
        e.ArrayField("Objects", p.Objects, 7, [&](Encoder& item, const SnapshotObject& value) noexcept
        {
            item.U32("KeyIndex", t.Keys.Add(item, value.Key));
            item.Bool("HasTypeName", value.TypeName != nullptr); item.Text("TypeName", value.TypeName);
            item.Bool("HasAddress", value.Address.Present); item.U64("Address", value.Address.Present ? value.Address.Value : 0);
            item.U32("SizeIndex", t.Values.Add(item, value.Size));
            item.U32("ClassificationIndex", value.TypeClassification.Present ? t.Classifications.Add(item, value.TypeClassification.Value) : 0);
        });
        e.ArrayField("Ownership", p.Ownership, 5, [&](Encoder& item, const OwnershipEdge& value) noexcept
        {
            item.U32("FromIndex", t.Keys.Add(item, value.From)); item.U32("ToIndex", t.Keys.Add(item, value.To));
            item.Text("Relationship", value.Relationship); item.U8("Evidence", value.Evidence); item.U32("QualityIndex", t.Qualities.Add(item, value.DataQuality));
        });
        e.ArrayField("Tree", p.Tree, 7, [&](Encoder& item, const TreeEdge& value) noexcept
        {
            item.U32("ParentIndex", value.Parent.Present ? t.Keys.Add(item, value.Parent.Value) : 0);
            item.U32("ChildIndex", t.Keys.Add(item, value.Child)); item.U8("Relationship", value.Relationship);
            item.I32("Order", value.Order); item.Bool("Connected", value.Connected);
            item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause)); item.U8("Membership", value.Membership);
        });
        e.ArrayField("Correlations", p.Correlations, 5, [&](Encoder& item, const Correlation& value) noexcept
        {
            item.U32("FromIndex", t.Keys.Add(item, value.From)); item.U32("ToIndex", t.Keys.Add(item, value.To));
            item.U8("Kind", value.Kind); item.Bool("Linked", value.Linked); item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause));
        });
        e.ArrayField("Resources", p.Resources, 16, [&](Encoder& item, const Resource& value) noexcept
        {
            item.U32("ResourceIndex", t.Keys.Add(item, value.Object)); item.U8("Operation", value.Operation); item.Guid("DeviceEpoch", value.DeviceEpoch);
            item.Bool("HasBackingAllocationId", value.BackingAllocationId.Present); item.Guid("BackingAllocationId", value.BackingAllocationId.Present ? value.BackingAllocationId.Value : GUID{});
            item.Bool("HasAllocationBytes", value.AllocationBytes.Present); item.U64("AllocationBytes", value.AllocationBytes.Present ? value.AllocationBytes.Value : 0);
            item.U8("SizeProvenance", value.SizeProvenance); item.U32("OwnerIndex", value.Owner.Present ? t.Keys.Add(item, value.Owner.Value) : 0);
            item.Bool("HasWidthPixels", value.WidthPixels.Present); item.U32("WidthPixels", value.WidthPixels.Present ? value.WidthPixels.Value : 0);
            item.Bool("HasHeightPixels", value.HeightPixels.Present); item.U32("HeightPixels", value.HeightPixels.Present ? value.HeightPixels.Value : 0);
            item.U32("QualityIndex", t.Qualities.Add(item, value.DataQuality));
            item.U32("ClassificationIndex", t.Classifications.Add(item, value.ResourceClassification)); item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause));
        });
        e.ArrayField("Visuals", p.Visuals, 4, [&](Encoder& item, const VisualDelta& value) noexcept
        {
            item.U32("VisualIndex", t.Keys.Add(item, value.Visual)); item.U8("Property", value.Property);
            item.U32("ValueIndex", t.Values.Add(item, value.PropertyValue)); item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause));
        });
        e.ArrayField("Associations", p.Associations, 4, [&](Encoder& item, const CorePeerAssociation& value) noexcept
        {
            item.U32("CoreIndex", t.Keys.Add(item, value.Core)); item.U32("PeerIndex", t.Keys.Add(item, value.Peer));
            item.Bool("Linked", value.Linked); item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause));
        });
        e.ArrayField("Types", p.Types, 3, [&](Encoder& item, const TypeResolved& value) noexcept
        {
            item.U64("TypeId", value.TypeId); item.Text("Name", value.Name);
            item.U32("ClassificationIndex", value.TypeClassification.Present ? t.Classifications.Add(item, value.TypeClassification.Value) : 0);
        });
        e.ArrayField("Allocations", p.Allocations, 12, [&](Encoder& item, const Allocation& value) noexcept
        {
            item.U64("AllocationId", value.AllocationId); item.U8("Operation", value.Operation);
            item.Bool("HasPreviousAllocationId", value.PreviousAllocationId.Present); item.U64("PreviousAllocationId", value.PreviousAllocationId.Present ? value.PreviousAllocationId.Value : 0);
            item.U64("RequestedBytes", value.RequestedBytes); item.Bool("HasActualBytes", value.ActualBytes.Present); item.U64("ActualBytes", value.ActualBytes.Present ? value.ActualBytes.Value : 0);
            item.Guid("AllocatorEpoch", value.AllocatorEpoch); item.Bool("HasBaselineCaptureId", value.BaselineCaptureId.Present); item.Guid("BaselineCaptureId", value.BaselineCaptureId.Present ? value.BaselineCaptureId.Value : GUID{});
            item.U32("QualityIndex", t.Qualities.Add(item, value.DataQuality)); item.U32("CauseIndex", t.Causes.Add(item, value.ChangeCause));
        });
        t.Keys.Write(e, "Keys", 2, [](Encoder& item, const ObjectIdentity& value) noexcept
        {
            item.U8("Domain", static_cast<uint8_t>(value.IdentityDomain)); item.U64("RuntimeObjectId", value.Id);
        });
        t.Values.Write(e, "Values", 7, [&](Encoder& item, const Value& value) noexcept
        {
            item.U8("Kind", value.Kind); item.U8("Unit", value.Unit); item.I64("Integer", value.Integer);
            item.F64("Number", value.Number); item.Bool("Boolean", value.Boolean); item.Text("Text", value.Text);
            item.U32("QualityIndex", t.Qualities.Add(item, value.DataQuality));
        });
        t.Classifications.Write(e, "Classifications", 3, [](Encoder& item, const Classification& value) noexcept
        {
            item.U8("Kind", value.Kind); item.U8("Evidence", value.Evidence); item.Text("Source", value.Source);
        });
        t.Causes.Write(e, "Causes", 7, [](Encoder& item, const Cause& value) noexcept
        {
            item.U8("Kind", value.Kind); item.Bool("HasAttempt", value.Reference.Present);
            const Attempt attempt = value.Reference.Present ? value.Reference.Value : Attempt{};
            item.U64("AttemptId", attempt.Id); item.Guid("RootId", attempt.RootId); item.U64("DispatcherId", attempt.DispatcherId);
            item.Bool("HasReason", value.Reason != nullptr); item.Text("Reason", value.Reason);
        });
        t.Qualities.Write(e, "Qualities", 4, [](Encoder& item, const Quality& value) noexcept
        {
            item.U8("Kind", value.Kind); item.U8("Coverage", value.Coverage);
            item.Bool("HasReason", value.Reason != nullptr); item.Text("Reason", value.Reason);
        });
        return t.Keys.Overflow || t.Values.Overflow || t.Classifications.Overflow || t.Causes.Overflow || t.Qualities.Overflow ?
            HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) : S_OK;
    }
    inline void Encode(Encoder& e, const Loss& p) noexcept
    {
        e.Struct("Interval", 2); e.I64("StartQpc", p.StartQpc); e.I64("EndQpc", p.EndQpc);
        e.OptionalField("LostEvents", p.LostEvents, U64Field); e.Text("Reason", p.Reason); e.Bool("HasDomains", p.HasDomains);
        e.ArrayField("Domains", p.HasDomains ? p.Domains : Array<DomainReference>{}, 2, DomainField);
    }
    inline void Encode(Encoder& e, const XamlPropertyChanged& p) noexcept
    {
        ObjectField(e, "Object", p.Object); e.Struct("Property", 3);
        e.U64("PropertyId", p.PropertyId); e.Text("DeclaringType", p.DeclaringType); e.Text("Name", p.Name);
        ValueField(e, "Previous", p.Previous); ValueField(e, "Current", p.Current); CauseField(e, p.ChangeCause);
    }
    inline void Encode(Encoder& e, const RuntimeCapabilities& p) noexcept
    {
        e.U32("CapabilityCount", p.Capabilities.Count);
        e.ArrayField("Capabilities", p.Capabilities, 3, [](Encoder& item, const CapabilityEntry& entry) noexcept
        {
            item.U8("Capability", entry.Capability); item.I32("Version", entry.Version);
            QualityField(item, "Quality", entry.DataQuality);
        });
    }
}
